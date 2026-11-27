#include "girassol/world.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace girassol {
namespace {
constexpr double kPi = 3.14159265358979323846;
double clamp01(double x) { return std::clamp(x, 0.0, 1.0); }
double finite_clamp(double x, double lo, double hi) { return std::clamp(x, lo, hi); }
std::size_t ix(Guild g) { return static_cast<std::size_t>(g); }
std::size_t ix(Pathogen p) { return static_cast<std::size_t>(p); }
}

WorldState WorldState::pristine() { return WorldState{}; }

bool WorldState::valid() const noexcept {
    const auto finite = [](double x) { return std::isfinite(x); };
    if (!finite(hour_of_day) || hour_of_day < 0.0 || hour_of_day >= 24.0) return false;
    const double climate_values[] = {climate.temperature_c, climate.pressure_hpa,
        climate.relative_humidity, climate.wind_m_s, climate.cloud_cover,
        climate.precipitation, climate.evapotranspiration};
    for (double x : climate_values) if (!finite(x)) return false;
    if (climate.pressure_hpa <= 0.0 || climate.wind_m_s < 0.0) return false;
    if (climate.relative_humidity < 0.0 || climate.relative_humidity > 1.0 ||
        climate.cloud_cover < 0.0 || climate.cloud_cover > 1.0) return false;
    const double eco[] = {ecosystem.soil_moisture, ecosystem.surface_water,
        ecosystem.soil_fertility, ecosystem.native_canopy, ecosystem.detritus};
    for (double x : eco) if (!finite(x) || x < 0.0 || x > 1.0) return false;
    for (double x : ecosystem.guild_density) if (!finite(x) || x < 0.0) return false;
    for (const auto& p : pathogens) {
        if (!finite(p.susceptible) || !finite(p.infected) || !finite(p.recovered) || !finite(p.immunity) ||
            p.susceptible < 0.0 || p.infected < 0.0 || p.recovered < 0.0 ||
            p.immunity < 0.0 || p.immunity > 1.0) return false;
        if (p.susceptible + p.infected + p.recovered > 1.000001) return false;
    }
    for (const auto& c : citizens) {
        const double values[] = {c.hunger,c.thirst,c.curiosity,c.wellbeing,c.nutrition_reserve,c.hydration_reserve};
        for (double x : values) if (!finite(x) || x < 0.0 || x > 1.0) return false;
    }
    return true;
}

WorldEngine::WorldEngine(WorldState initial) : state_(std::move(initial)) {
    if (!state_.valid()) throw std::invalid_argument("invalid initial world state");
}

void WorldEngine::step(double hours) {
    if (!std::isfinite(hours) || hours <= 0.0 || hours > 24.0)
        throw std::invalid_argument("step duration must be in (0, 24] hours");
    if (!state_.valid()) throw std::runtime_error("world state invariant violated");
    const double dt = hours / 24.0;
    const double old_canopy = state_.ecosystem.native_canopy;
    const double old_water = state_.ecosystem.surface_water;
    const double day_phase = (state_.hour_of_day + hours * 0.5) / 24.0;
    const double solar = std::sin(2.0 * kPi * (day_phase - 0.25));

    Climate& c = state_.climate;
    const double target_temp = 24.0 + 7.0 * solar - 3.0 * c.cloud_cover;
    c.temperature_c += (target_temp - c.temperature_c) * (1.0 - std::exp(-hours / 36.0));
    const double barometric = 1013.25 + 2.0 * std::sin(2.0 * kPi * day_phase);
    c.pressure_hpa += (barometric - c.pressure_hpa) * (1.0 - std::exp(-hours / 18.0));
    const double wind_target = 0.8 + 3.0 * c.cloud_cover + 0.4 * (1.0 - old_canopy);
    c.wind_m_s += (wind_target - c.wind_m_s) * (1.0 - std::exp(-hours / 12.0));
    c.precipitation = clamp01(0.08 + 0.20*c.cloud_cover + 0.08*c.relative_humidity - 0.10*clamp01(c.wind_m_s/12.0));
    c.evapotranspiration = clamp01(0.02 + 0.09*std::max(c.temperature_c,0.0)/40.0 + 0.08*old_canopy + 0.02*clamp01(c.wind_m_s/12.0));
    c.relative_humidity = clamp01(c.relative_humidity + dt*(0.10*c.precipitation - 0.035*c.evapotranspiration - 0.01*std::max(c.temperature_c-25.0,0.0)/10.0));
    c.cloud_cover = clamp01(c.cloud_cover + dt*(0.10*c.relative_humidity - 0.07*c.evapotranspiration));

    Ecosystem& e = state_.ecosystem;
    const double infiltration = c.precipitation * (0.45 + 0.45*e.native_canopy);
    const double runoff = std::max(0.0, c.precipitation - infiltration) * (0.25 + 0.5*(1.0-e.soil_moisture));
    const double et_soil = c.evapotranspiration * (0.35 + 0.65*e.native_canopy);
    e.surface_water = clamp01(e.surface_water + dt*(infiltration*0.45 - et_soil*0.30 - runoff*0.25));
    e.soil_moisture = clamp01(e.soil_moisture + dt*(infiltration - et_soil - runoff));
    e.detritus = clamp01(e.detritus + dt*(0.025*e.native_canopy - 0.035*e.guild_density[ix(Guild::Decomposers)]));
    e.soil_fertility = clamp01(e.soil_fertility + dt*(0.025*e.detritus - 0.018*(1.0-e.native_canopy)));
    const double vegetation_growth = 0.035*e.soil_moisture*e.soil_fertility*(1.0-e.native_canopy);
    const double vegetation_stress = 0.018*std::max(0.0,0.35-e.soil_moisture) + 0.006*e.guild_density[ix(Guild::Herbivores)];
    e.native_canopy = clamp01(e.native_canopy + dt*(vegetation_growth-vegetation_stress));

    auto& g = e.guild_density;
    const double plants = e.native_canopy*e.soil_fertility;
    const double capacity[] = {0.15+0.75*plants, 0.08+0.45*g[ix(Guild::Herbivores)],
        0.10+0.75*e.native_canopy, 0.10+0.70*e.detritus,
        0.05+0.70*e.surface_water, 0.08+0.65*e.surface_water};
    const double rates[] = {0.10,0.08,0.12,0.11,0.14,0.09};
    for (std::size_t i=0; i<g.size(); ++i) {
        const double ratio = g[i] / std::max(0.05, capacity[i]);
        double derivative = rates[i]*g[i]*(1.0-ratio);
        if (i == ix(Guild::Herbivores)) derivative -= 0.025*g[i]*g[ix(Guild::Predators)];
        if (i == ix(Guild::Predators)) derivative += 0.018*g[i]*g[ix(Guild::Herbivores)];
        if (i == ix(Guild::Pollinators)) derivative += 0.01*g[i]*e.native_canopy;
        if (i == ix(Guild::Vectors)) derivative -= 0.02*g[i]*(1.0-e.surface_water);
        g[i] = std::max(0.0, g[i] + dt*derivative);
    }

    for (std::size_t i=0; i<state_.pathogens.size(); ++i) {
        auto& p = state_.pathogens[i];
        const double contact = 0.12 + 0.20*g[ix(Guild::Herbivores)] + 0.10*g[ix(Guild::Vectors)];
        const double water_route = i == ix(Pathogen::Viral) ? 0.35 : 0.70;
        const double soil_route = i == ix(Pathogen::Parasitic) ? 0.75 : 0.25;
        const double vector_route = i == ix(Pathogen::Viral) ? 0.15 : 0.65;
        const double hazard = (0.018 + 0.008*static_cast<double>(i)) *
            (contact + water_route*e.surface_water + soil_route*(1.0-e.soil_fertility) + vector_route*g[ix(Guild::Vectors)]);
        const double susceptible_fraction = clamp01(p.susceptible*(1.0-p.immunity));
        const double new_cases = std::min(p.susceptible, susceptible_fraction*(1.0-std::exp(-hazard*hours)));
        const double recovery = std::min(p.infected, p.infected*(1.0-std::exp(-0.035*hours)));
        p.susceptible -= new_cases;
        p.infected += new_cases-recovery;
        p.recovered = clamp01(p.recovered+recovery);
        p.immunity = clamp01(p.immunity+0.002*hours*recovery);
    }

    const double food_access = clamp01(0.35*plants + 0.20*g[ix(Guild::Herbivores)] + 0.15*e.detritus);
    const double water_access = clamp01(0.25+0.75*e.surface_water);
    for (auto& person : state_.citizens) {
        const double disease = 0.0 + state_.pathogens[0].infected + state_.pathogens[1].infected + state_.pathogens[2].infected;
        person.nutrition_reserve = clamp01(person.nutrition_reserve + hours*(0.002*food_access-0.003));
        person.hydration_reserve = clamp01(person.hydration_reserve + hours*(0.002*water_access-0.004));
        person.hunger = clamp01(person.hunger + hours*(0.025-0.018*food_access+0.01*disease));
        person.thirst = clamp01(person.thirst + hours*(0.032-0.022*water_access+0.012*disease));
        person.curiosity = clamp01(person.curiosity + hours*(0.004*person.wellbeing-0.002*person.curiosity));
        person.wellbeing = clamp01(person.wellbeing + hours*(0.0015*(1.0-person.hunger-person.thirst)-0.0008*disease));
    }

    state_.hour_of_day += hours;
    while (state_.hour_of_day >= 24.0) {
        state_.hour_of_day -= 24.0;
        ++state_.day;
    }
    if (!state_.valid()) throw std::runtime_error("world step produced invalid state");
}

std::string WorldEngine::snapshot_json() const {
    std::ostringstream out;
    out << std::setprecision(8) << "{\"day\":" << state_.day << ",\"hour\":" << state_.hour_of_day
        << ",\"temperatureC\":" << state_.climate.temperature_c << ",\"humidity\":" << state_.climate.relative_humidity
        << ",\"precipitation\":" << state_.climate.precipitation << ",\"surfaceWater\":" << state_.ecosystem.surface_water
        << ",\"canopy\":" << state_.ecosystem.native_canopy << ",\"infected\":[";
    for (std::size_t i=0;i<state_.pathogens.size();++i) {
        if (i) out << ',';
        out << state_.pathogens[i].infected;
    }
    out << "],\"citizens\":[";
    for (std::size_t i=0;i<state_.citizens.size();++i) {
        if (i) out << ',';
        const auto& c = state_.citizens[i];
        out << "{\"id\":" << c.id << ",\"hunger\":" << c.hunger << ",\"thirst\":" << c.thirst
            << ",\"curiosity\":" << c.curiosity << ",\"mute\":" << (c.mute?"true":"false")
            << ",\"deaf\":" << (c.deaf?"true":"false") << '}';
    }
    out << "]}";
    return out.str();
}

std::string WorldEngine::local_dialogue(const WorldState& state, std::uint64_t citizen_id) {
    const auto it = std::find_if(state.citizens.begin(), state.citizens.end(),
        [citizen_id](const Citizen& c) { return c.id == citizen_id; });
    if (it == state.citizens.end()) return "Ainda não há ninguém por aqui.";
    if (it->thirst > 0.70) return "Estou com sede; vou procurar água segura.";
    if (it->hunger > 0.70) return "Preciso encontrar alimento.";
    if (it->mute) return "(expressão por gesto ou texto) Estou bem e quero explorar.";
    return "Que mudanças a mata trouxe hoje?";
}
} // namespace girassol
