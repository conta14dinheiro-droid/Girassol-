#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace girassol::omega {

struct SoilColumn {
    double surface_water{0.25};
    double topsoil{0.55};
    double subsoil{0.60};
    double groundwater{0.45};
    double fertility{0.65};
    double slope{0.08};
    double canopy{0.75};
};
struct HydroForcing {
    double rain{0.02};
    double evapotranspiration{0.01};
    double infiltration_rate{0.25};
    double drainage_rate{0.08};
};
SoilColumn hydrology_step(SoilColumn state, HydroForcing forcing, double dt_days);

double rothermel_reduced_ros(double base_ros_m_s, double fuel_moisture,
                             double wind_m_s, double slope_radians,
                             double canopy_density);
double gompertz_makeham_hazard(double age_years, double makeham,
                               double gompertz_scale, double gompertz_rate,
                               double environmental_hazard);
std::vector<double> leontief_output(const std::vector<std::vector<double>>& input,
                                   const std::vector<double>& final_demand);

struct Point2 { double x{}; double y{}; };
std::vector<Point2> fabrik_solve(std::span<const Point2> joints, Point2 target,
                                 std::size_t iterations, double tolerance);

struct TlvField { std::uint16_t tag{}; std::vector<std::uint8_t> value; };
std::vector<std::uint8_t> tlv_encode(std::span<const TlvField> fields);
std::vector<TlvField> tlv_decode(std::span<const std::uint8_t> bytes);
std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept;

struct LoraPolicy {
    double max_false_positive_rate{0.05};
    double confidence_delta{0.01};
    double max_kl_nats{0.02};
    std::size_t max_adapter_bytes{64U * 1024U * 1024U};
    double max_latency_ms{120.0};
    double max_temperature_c{42.0};
};
struct LoraEvidence {
    std::size_t trials{};
    std::size_t false_positives{};
    double reference_probability{0.5};
    double candidate_probability{0.5};
    double adapter_bytes{};
    double latency_ms{};
    double temperature_c{};
    bool scheduler_permits{false};
};
struct PromotionResult { bool promote{}; double upper_false_positive_bound{}; double kl_nats{}; };
PromotionResult validate_lora_promotion(const LoraEvidence& evidence,
                                        const LoraPolicy& policy);

enum class GraphicsTier { Gb2, Gb4, Gb8, Gb12Plus };
struct GraphicsProfile {
    GraphicsTier tier{};
    double render_scale{};
    double view_distance{};
    double vegetation_density{};
    bool frame_generation{};
};
GraphicsProfile graphics_profile(std::uint64_t total_ram_bytes, double temperature_c);

struct SemanticResult { std::string action; std::string explanation; bool used_local_model{}; };
SemanticResult grs_semantic_loop(std::string_view observation);
std::string omega_status_json(std::uint64_t total_ram_bytes, double temperature_c);

} // namespace girassol::omega
