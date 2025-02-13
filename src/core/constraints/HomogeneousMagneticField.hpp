/*
 * Copyright (C) 2010-2022 The ESPResSo project
 *
 * This file is part of ESPResSo.
 *
 * ESPResSo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ESPResSo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Constraint.hpp"
#include "Observable_stat.hpp"
#include "Particle.hpp"

#include <utils/Vector.hpp>

namespace Constraints {

class HomogeneousMagneticField : public Constraint {
public:
  HomogeneousMagneticField() : m_field({1., 0., 0.}) {}

  void set_H(Utils::Vector3d const &H) { m_field = H; }

  Utils::Vector3d const &H() const { return m_field; }

  void add_energy(const Particle &p, const Utils::Vector3d &, double,
                  Observable_stat &energy) const override;

  ParticleForce force(const Particle &p, const Utils::Vector3d &,
                      double) override;

  bool fits_in_box(Utils::Vector3d const &) const override { return true; }

private:
  Utils::Vector3d m_field;
};

} // namespace Constraints
