#ifndef GIRASSOL_WORLD_HPP
#define GIRASSOL_WORLD_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace girassol {

enum class Guild : std::size_t {
    Herbivores = 0, Predators, Pollinators, Decomposers, Vectors, Aquatic, Count
};
enum class Pathogen : std::size_t { Viral = 0, Bacterial, Parasitic, Count };

struct Climate {
    double temperature_c{24.0};
    double pressure_hpa{1013.25};
    double relative_humidity{0.65};
    double wind_m_s{1.5};
    double cloud_cover{0.35};
    double precipitation{0.0};
    double evapotranspiration{0.0};
};
struct Ecosystem {
    double soil_moisture{0.62};
    double surface_water{0.45};
    double soil_fertility{0.72};
    double native_canopy{0.85};
    double detritus{0.25};
    std::array<double, static_cast<std::size_t>(Guild::Count)> guild_density{
        0.35, 0.12, 0.55, 0.48, 0.18, 0.30
    };
};
struct PathogenState {
    double susceptible{1.0};
    double infected{0.0};
    double recovered{0.0};
    double immunity{0.15};
};
struct Citizen {
    std::uint64_t id{0};
    double hunger{0.0};
    double thirst{0.0};
    double curiosity{0.5};
    double wellbeing{0.75};
    double nutrition_reserve{0.7};
    double hydration_reserve{0.7};
    bool mute{false};
    bool deaf{false};
};
struct WorldState {
    std::uint64_t day{0};
    double hour_of_day{0.0};
    Climate climate{};
    Ecosystem ecosystem{};
    std::array<PathogenState, static_cast<std::size_t>(Pathogen::Count)> pathogens{};
    std::vector<Citizen> citizens{};

    static WorldState pristine();
    bool valid() const noexcept;
};

class WorldEngine {
public:
    explicit WorldEngine(WorldState initial = WorldState::pristine());
    const WorldState& state() const noexcept { return state_; }
    void step(double hours);
    std::string snapshot_json() const;
    static std::string local_dialogue(const WorldState& state, std::uint64_t citizen_id);
private:
    WorldState state_;
};

} // namespace girassol
#endif
