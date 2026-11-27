#include "girassol/world.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

using namespace girassol;

int main() {
    WorldEngine pristine;
    assert(pristine.state().citizens.empty());
    assert(pristine.state().ecosystem.native_canopy > 0.0);
    assert(pristine.state().valid());

    bool rejected = false;
    try { pristine.step(0.0); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { pristine.step(25.0); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);

    WorldState populated = WorldState::pristine();
    populated.citizens.push_back(Citizen{.id=42});
    populated.pathogens[static_cast<std::size_t>(Pathogen::Bacterial)].infected = 0.08;
    populated.pathogens[static_cast<std::size_t>(Pathogen::Bacterial)].susceptible = 0.82;
    WorldEngine a(populated), b(populated);
    for (int i=0;i<12;++i) { a.step(2.0); b.step(2.0); }
    assert(a.state().valid() && b.state().valid());
    assert(a.snapshot_json() == b.snapshot_json());
    assert(a.state().day == 1);
    const Citizen& person = a.state().citizens.front();
    assert(person.hunger >= 0.0 && person.hunger <= 1.0);
    assert(person.thirst >= 0.0 && person.thirst <= 1.0);
    assert(std::isfinite(a.state().climate.temperature_c));
    assert(a.state().ecosystem.native_canopy >= 0.0 && a.state().ecosystem.native_canopy <= 1.0);
    assert(WorldEngine::local_dialogue(a.state(), 42).size() > 0);

    WorldState invalid = WorldState::pristine();
    invalid.ecosystem.surface_water = std::nan("");
    rejected = false;
    try { WorldEngine bad(invalid); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    return 0;
}
