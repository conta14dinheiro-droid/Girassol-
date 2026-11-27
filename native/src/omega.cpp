#include "girassol/omega.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace girassol::omega {
namespace {
constexpr double kEpsilon = 1e-12;
double bounded(double value, double lo, double hi) {
    if (!std::isfinite(value)) throw std::invalid_argument("non-finite model input");
    return std::clamp(value, lo, hi);
}
double distance(Point2 a, Point2 b) { return std::hypot(a.x - b.x, a.y - b.y); }
void append_u16(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffU));
    out.push_back(static_cast<std::uint8_t>((v >> 8U) & 0xffU));
}
void append_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (unsigned shift = 0; shift < 32U; shift += 8U)
        out.push_back(static_cast<std::uint8_t>((v >> shift) & 0xffU));
}
std::uint16_t read_u16(std::span<const std::uint8_t> in, std::size_t at) {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(in[at]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(in[at + 1U]) << 8U));
}
std::uint32_t read_u32(std::span<const std::uint8_t> in, std::size_t at) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4U; ++i)
        value |= static_cast<std::uint32_t>(in[at + i]) << (8U * i);
    return value;
}
}

SoilColumn hydrology_step(SoilColumn s, HydroForcing f, double dt) {
    if (!std::isfinite(dt) || dt <= 0.0 || dt > 1.0)
        throw std::invalid_argument("hydrology dt must be in (0,1] day");
    s.surface_water = bounded(s.surface_water, 0.0, 1.0);
    s.topsoil = bounded(s.topsoil, 0.0, 1.0);
    s.subsoil = bounded(s.subsoil, 0.0, 1.0);
    s.groundwater = bounded(s.groundwater, 0.0, 1.0);
    s.fertility = bounded(s.fertility, 0.0, 1.0);
    s.slope = bounded(s.slope, 0.0, 1.0);
    s.canopy = bounded(s.canopy, 0.0, 1.0);
    f.rain = bounded(f.rain, 0.0, 1.0);
    f.evapotranspiration = bounded(f.evapotranspiration, 0.0, 1.0);
    f.infiltration_rate = bounded(f.infiltration_rate, 0.0, 1.0);
    f.drainage_rate = bounded(f.drainage_rate, 0.0, 1.0);
    const double interception = f.rain * (0.15 + 0.65 * s.canopy);
    const double to_top = std::min(s.surface_water + dt * interception,
        dt * f.infiltration_rate * (1.0 - s.topsoil) * (1.0 - 0.5 * s.slope));
    const double to_sub = std::min(s.topsoil, dt * f.drainage_rate * s.topsoil);
    const double to_ground = std::min(s.subsoil, dt * 0.5 * f.drainage_rate * s.subsoil);
    const double runoff = std::min(s.surface_water + dt * f.rain - to_top,
        dt * f.rain * (0.1 + 0.8 * s.slope) * (1.0 - s.canopy));
    const double evap = std::min(s.topsoil + to_top, dt * f.evapotranspiration);
    s.surface_water = bounded(s.surface_water + dt * f.rain - to_top - runoff, 0.0, 1.0);
    s.topsoil = bounded(s.topsoil + to_top - to_sub - evap, 0.0, 1.0);
    s.subsoil = bounded(s.subsoil + to_sub - to_ground, 0.0, 1.0);
    s.groundwater = bounded(s.groundwater + to_ground, 0.0, 1.0);
    s.fertility = bounded(s.fertility + dt * (0.01 * s.topsoil * s.canopy - 0.006 * s.slope * (1.0-s.canopy)), 0.0, 1.0);
    return s;
}

double rothermel_reduced_ros(double base, double moisture, double wind, double slope, double canopy) {
    if (!std::isfinite(base) || !std::isfinite(moisture) || !std::isfinite(wind) ||
        !std::isfinite(slope) || !std::isfinite(canopy) || base < 0.0 || wind < 0.0)
        throw std::invalid_argument("invalid fire model input");
    const double moisture_damping = std::exp(-3.0 * bounded(moisture, 0.0, 1.0));
    const double wind_factor = 0.08 * std::min(wind, 40.0);
    const double slope_factor = 3.0 * std::pow(std::tan(bounded(slope, -1.2, 1.2)), 2.0);
    const double fuel_continuity = 0.25 + 0.75 * bounded(canopy, 0.0, 1.0);
    return std::clamp(base * moisture_damping * (1.0 + wind_factor + slope_factor) * fuel_continuity,
                      0.0, 20.0);
}

double gompertz_makeham_hazard(double age, double makeham, double scale, double rate, double environmental) {
    if (!std::isfinite(age) || !std::isfinite(makeham) || !std::isfinite(scale) ||
        !std::isfinite(rate) || !std::isfinite(environmental) || age < 0.0 ||
        makeham < 0.0 || scale < 0.0 || rate < 0.0 || environmental < 0.0)
        throw std::invalid_argument("invalid mortality parameter");
    const double exponent = std::min(rate * age, 700.0);
    return std::min(makeham + scale * std::expm1(exponent) + environmental, 1e6);
}

std::vector<double> leontief_output(const std::vector<std::vector<double>>& a,
                                    const std::vector<double>& demand) {
    const std::size_t n = demand.size();
    if (n == 0U || a.size() != n) throw std::invalid_argument("Leontief matrix dimensions mismatch");
    std::vector<std::vector<double>> m(n, std::vector<double>(n + 1U));
    for (std::size_t r=0; r<n; ++r) {
        if (a[r].size() != n || !std::isfinite(demand[r]) || demand[r] < 0.0)
            throw std::invalid_argument("invalid Leontief row or demand");
        for (std::size_t c=0; c<n; ++c) {
            if (!std::isfinite(a[r][c]) || a[r][c] < 0.0) throw std::invalid_argument("invalid input coefficient");
            m[r][c] = (r == c ? 1.0 : 0.0) - a[r][c];
        }
        m[r][n] = demand[r];
    }
    for (std::size_t p=0; p<n; ++p) {
        std::size_t pivot = p;
        for (std::size_t r=p+1U; r<n; ++r) if (std::abs(m[r][p]) > std::abs(m[pivot][p])) pivot = r;
        if (std::abs(m[pivot][p]) < kEpsilon) throw std::domain_error("singular Leontief system");
        std::swap(m[p], m[pivot]);
        const double divisor = m[p][p];
        for (std::size_t c=p; c<=n; ++c) m[p][c] /= divisor;
        for (std::size_t r=0; r<n; ++r) if (r != p) {
            const double factor = m[r][p];
            for (std::size_t c=p; c<=n; ++c) m[r][c] -= factor * m[p][c];
        }
    }
    std::vector<double> result(n);
    for (std::size_t i=0; i<n; ++i) {
        result[i] = m[i][n];
        if (!std::isfinite(result[i]) || result[i] < -1e-9) throw std::domain_error("nonphysical Leontief output");
        result[i] = std::max(0.0, result[i]);
    }
    return result;
}

std::vector<Point2> fabrik_solve(std::span<const Point2> input, Point2 target, std::size_t iterations, double tol) {
    if (input.size() < 2U || !std::isfinite(target.x) || !std::isfinite(target.y) || !std::isfinite(tol) || tol < 0.0)
        throw std::invalid_argument("invalid FABRIK input");
    std::vector<Point2> p(input.begin(), input.end());
    std::vector<double> lengths;
    lengths.reserve(p.size()-1U);
    double reach = 0.0;
    for (std::size_t i=0; i+1U<p.size(); ++i) {
        if (!std::isfinite(p[i].x) || !std::isfinite(p[i].y)) throw std::invalid_argument("non-finite joint");
        lengths.push_back(distance(p[i], p[i+1U])); reach += lengths.back();
    }
    if (!std::isfinite(p.back().x) || !std::isfinite(p.back().y)) throw std::invalid_argument("non-finite joint");
    const Point2 root = p.front();
    if (distance(root, target) >= reach) {
        for (std::size_t i=0; i+1U<p.size(); ++i) {
            const double d = distance(p[i], target);
            if (d > kEpsilon) { const double ratio = lengths[i] / d; p[i+1U] = {p[i].x + (target.x-p[i].x)*ratio, p[i].y + (target.y-p[i].y)*ratio}; }
        }
        return p;
    }
    for (std::size_t pass=0; pass<iterations && distance(p.back(), target)>tol; ++pass) {
        p.back() = target;
        for (std::size_t i=p.size()-1U; i>0U; --i) {
            const double d=distance(p[i],p[i-1U]); if (d>kEpsilon) { const double q=lengths[i-1U]/d; p[i-1U]={p[i].x+(p[i-1U].x-p[i].x)*q,p[i].y+(p[i-1U].y-p[i].y)*q}; }
        }
        p.front()=root;
        for (std::size_t i=0; i+1U<p.size(); ++i) {
            const double d=distance(p[i],p[i+1U]); if (d>kEpsilon) { const double q=lengths[i]/d; p[i+1U]={p[i].x+(p[i+1U].x-p[i].x)*q,p[i].y+(p[i+1U].y-p[i].y)*q}; }
        }
    }
    return p;
}

std::uint32_t crc32(std::span<const std::uint8_t> data) noexcept {
    std::uint32_t crc=0xffffffffU;
    for (const auto byte : data) { crc ^= byte; for (int bit=0; bit<8; ++bit) crc=(crc&1U)?(crc>>1U)^0xedb88320U:crc>>1U; }
    return ~crc;
}
std::vector<std::uint8_t> tlv_encode(std::span<const TlvField> fields) {
    if (fields.size()>65535U) throw std::length_error("too many TLV fields");
    std::vector<std::uint8_t> out{'G','S','O','3'}; append_u16(out,1U); append_u16(out,static_cast<std::uint16_t>(fields.size()));
    for (const auto& f:fields) { if (f.value.size()>std::numeric_limits<std::uint32_t>::max()) throw std::length_error("TLV value too large"); append_u16(out,f.tag); append_u32(out,static_cast<std::uint32_t>(f.value.size())); out.insert(out.end(),f.value.begin(),f.value.end()); }
    append_u32(out,crc32(out)); return out;
}
std::vector<TlvField> tlv_decode(std::span<const std::uint8_t> b) {
    if (b.size()<12U || b[0]!='G'||b[1]!='S'||b[2]!='O'||b[3]!='3'||read_u16(b,4U)!=1U) throw std::invalid_argument("invalid TLV header/version");
    const std::uint32_t expected=read_u32(b,b.size()-4U); if (crc32(b.first(b.size()-4U))!=expected) throw std::invalid_argument("TLV CRC mismatch");
    const auto count=read_u16(b,6U); std::size_t at=8U; const std::size_t end=b.size()-4U; std::vector<TlvField> fields; fields.reserve(count);
    for (std::uint16_t i=0; i<count; ++i) { if (at>end || end-at<6U) throw std::invalid_argument("truncated TLV entry"); const auto tag=read_u16(b,at); const auto len=read_u32(b,at+2U); at+=6U; if (static_cast<std::size_t>(len)>end-at) throw std::invalid_argument("truncated TLV payload"); fields.push_back({tag,{b.begin()+static_cast<std::ptrdiff_t>(at),b.begin()+static_cast<std::ptrdiff_t>(at+len)}}); at+=len; }
    if (at!=end) throw std::invalid_argument("unexpected trailing TLV data"); return fields;
}

PromotionResult validate_lora_promotion(const LoraEvidence& e, const LoraPolicy& p) {
    if (e.trials==0U || e.false_positives>e.trials || !std::isfinite(e.reference_probability) || !std::isfinite(e.candidate_probability) || !std::isfinite(e.adapter_bytes) || !std::isfinite(e.latency_ms) || !std::isfinite(e.temperature_c) || !std::isfinite(p.confidence_delta) || p.confidence_delta<=0.0 || p.confidence_delta>=1.0)
        throw std::invalid_argument("invalid LoRA validation evidence/policy");
    const double observed=static_cast<double>(e.false_positives)/static_cast<double>(e.trials);
    const double radius=std::sqrt(std::log(2.0/p.confidence_delta)/(2.0*static_cast<double>(e.trials)));
    const double upper=std::min(1.0,observed+radius);
    const double q=bounded(e.candidate_probability,1e-12,1.0-1e-12), r=bounded(e.reference_probability,1e-12,1.0-1e-12);
    const double kl=q*std::log(q/r)+(1.0-q)*std::log((1.0-q)/(1.0-r));
    const bool pass=upper<=p.max_false_positive_rate && kl<=p.max_kl_nats && e.adapter_bytes<=static_cast<double>(p.max_adapter_bytes) && e.latency_ms<=p.max_latency_ms && e.temperature_c<=p.max_temperature_c && e.scheduler_permits;
    return {pass,upper,kl};
}
GraphicsProfile graphics_profile(std::uint64_t ram, double temp) {
    if (!std::isfinite(temp)) throw std::invalid_argument("invalid device temperature");
    const auto gb=[](std::uint64_t n){return n/(1024ULL*1024ULL*1024ULL);};
    const std::uint64_t amount=gb(ram);
    GraphicsTier tier=amount>=12U?GraphicsTier::Gb12Plus:amount>=8U?GraphicsTier::Gb8:amount>=4U?GraphicsTier::Gb4:GraphicsTier::Gb2;
    double scale=tier==GraphicsTier::Gb2?0.65:tier==GraphicsTier::Gb4?0.78:tier==GraphicsTier::Gb8?0.90:1.0;
    double view=tier==GraphicsTier::Gb2?80.0:tier==GraphicsTier::Gb4?140.0:tier==GraphicsTier::Gb8?240.0:400.0;
    double density=tier==GraphicsTier::Gb2?0.35:tier==GraphicsTier::Gb4?0.55:tier==GraphicsTier::Gb8?0.78:1.0;
    if (temp>=42.0) { scale*=0.85; view*=0.8; density*=0.8; }
    return {tier,scale,view,density,amount>=8U && temp<40.0};
}
SemanticResult grs_semantic_loop(std::string_view observation) {
    if (observation.empty() || observation.size()>4096U) throw std::invalid_argument("semantic observation size outside bounds");
    // Deterministic, rule-based offline baseline. A llama.cpp adapter may replace this behind the same contract.
    if (observation.find("água")!=std::string_view::npos) return {"inspect_water", "Observação relacionada à água registrada para inspeção local.", false};
    if (observation.find("fogo")!=std::string_view::npos) return {"raise_fire_watch", "Sinal de fogo encaminhado à vigilância da simulação.", false};
    return {"record_observation", "Observação registrada; nenhuma inferência externa foi usada.", false};
}
std::string omega_status_json(std::uint64_t ram, double temperature) {
    const auto p=graphics_profile(ram,temperature);
    const char* tier=p.tier==GraphicsTier::Gb2?"2GB":p.tier==GraphicsTier::Gb4?"4GB":p.tier==GraphicsTier::Gb8?"8GB":"12GB+";
    return std::string("{\"tier\":\"")+tier+"\",\"renderScale\":"+std::to_string(p.render_scale)+",\"viewDistance\":"+std::to_string(p.view_distance)+",\"vegetationDensity\":"+std::to_string(p.vegetation_density)+",\"frameGeneration\":"+(p.frame_generation?"true":"false")+",\"silentVigil\":false,\"microscope\":\"available\"}";
}
} // namespace girassol::omega
