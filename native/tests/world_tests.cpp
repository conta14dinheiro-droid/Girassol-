#include "girassol/omega.hpp"
#include "girassol/world.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace girassol;
int main() {
    WorldEngine pristine;
    assert(pristine.state().citizens.empty());
    assert(pristine.state().ecosystem.native_canopy > 0.0);
    assert(pristine.state().valid());
    bool rejected=false;
    try { pristine.step(0.0); } catch (const std::invalid_argument&) { rejected=true; }
    assert(rejected);
    rejected=false;
    try { pristine.step(25.0); } catch (const std::invalid_argument&) { rejected=true; }
    assert(rejected);
    WorldState populated=WorldState::pristine(); populated.citizens.push_back(Citizen{.id=42});
    populated.pathogens[static_cast<std::size_t>(Pathogen::Bacterial)].infected=0.08;
    populated.pathogens[static_cast<std::size_t>(Pathogen::Bacterial)].susceptible=0.82;
    WorldEngine a(populated),b(populated); for(int i=0;i<12;++i){a.step(2.0);b.step(2.0);}
    assert(a.state().valid()&&b.state().valid()); assert(a.snapshot_json()==b.snapshot_json()); assert(a.state().day==1);
    assert(a.state().citizens.front().hunger>=0.0&&a.state().citizens.front().hunger<=1.0);
    assert(std::isfinite(a.state().climate.temperature_c));
    WorldState invalid=WorldState::pristine(); invalid.ecosystem.surface_water=std::nan("");
    rejected=false; try{WorldEngine bad(invalid);}catch(const std::invalid_argument&){rejected=true;} assert(rejected);

    using namespace girassol::omega;
    const auto wet=rothermel_reduced_ros(0.1,0.8,3.0,0.1,0.8);
    const auto dry=rothermel_reduced_ros(0.1,0.1,3.0,0.1,0.8); assert(dry>wet&&wet>=0.0);
    assert(gompertz_makeham_hazard(80.0,0.001,0.00001,0.08,0.002)>gompertz_makeham_hazard(20.0,0.001,0.00001,0.08,0.002));
    const auto out=leontief_output({{0.2,0.1},{0.1,0.2}},{1.0,2.0}); assert(out.size()==2U&&out[0]>1.0&&out[1]>2.0);
    SoilColumn soil{}; for(int i=0;i<100;++i) soil=hydrology_step(soil,{0.03,0.01,0.2,0.05},0.1); assert(soil.topsoil>=0.0&&soil.topsoil<=1.0&&soil.groundwater<=1.0);
    std::vector<TlvField> fields{{1U,{1U,2U,3U}},{7U,{9U}}}; const auto encoded=tlv_encode(fields); const auto decoded=tlv_decode(encoded); assert(decoded.size()==2U&&decoded[0].value==fields[0].value);
    auto corrupt=encoded; corrupt[9]^=1U; rejected=false; try{(void)tlv_decode(corrupt);}catch(const std::invalid_argument&){rejected=true;} assert(rejected);
    LoraPolicy policy; LoraEvidence evidence{100000U,0U,0.5,0.5,1024.0,25.0,35.0,true}; const auto promotion=validate_lora_promotion(evidence,policy); assert(promotion.promote&&promotion.upper_false_positive_bound<policy.max_false_positive_rate);
    evidence.false_positives=1000U; assert(!validate_lora_promotion(evidence,policy).promote);
    const auto two=graphics_profile(2ULL*1024ULL*1024ULL*1024ULL,35.0); const auto four=graphics_profile(4ULL*1024ULL*1024ULL*1024ULL,35.0); const auto eight=graphics_profile(8ULL*1024ULL*1024ULL*1024ULL,35.0); const auto twelve=graphics_profile(16ULL*1024ULL*1024ULL*1024ULL,35.0);
    assert(two.render_scale<four.render_scale&&four.render_scale<eight.render_scale&&eight.render_scale<twelve.render_scale);
    const Point2 joints[]{{0,0},{1,0},{2,0}}; const auto solved=fabrik_solve(joints,{1.0,1.0},32U,1e-5); assert(solved.size()==3U);
    assert(grs_semantic_loop("observar água").action=="inspect_water");
    return 0;
}
