# Copyright (C) 2019-2022 The ESPResSo project
#
# This file is part of ESPResSo.
#
# ESPResSo is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ESPResSo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import unittest as ut
import importlib_wrapper
import numpy as np
from scipy import constants

params = {'N_SAMPLES_EQUIL': 25, 'N_SAMPLES_PROD': 5,
          'N_SAMPLES_EQUIL_CAP': 5, 'N_SAMPLES_CAP': 1,
          'MIN_PHI': 1, 'MAX_PHI': 2.5, 'N_PHI': 4}

tutorial, skipIfMissingFeatures = importlib_wrapper.configure_and_import(
    "@TUTORIALS_DIR@/electrodes/electrodes_part2.py",
    script_suffix="@TEST_SUFFIX@", **params)


@skipIfMissingFeatures
class Tutorial(ut.TestCase):

    def test_potential_difference(self):
        # Test that the applied potential difference equals the one from
        # integrating Poisson equation (within 30 %)
        msg = 'The potential difference is not equal to the one from integrating Poisson equation.'
        self.assertAlmostEqual(
            tutorial.measured_potential_difference / tutorial.POTENTIAL_DIFF, 1, delta=0.1, msg=msg)

    def test_charge_profile(self):
        # Roughly test the profile, deviations are expected!!
        charge_profile = (
            tutorial.cation_profile_mean +
            tutorial.anion_profile_mean)
        analytic = (tutorial.gouy_chapman_density(tutorial.zs, tutorial.CONCENTRATION, tutorial.DEBYE_LENGTH, -tutorial.POTENTIAL_DIFF / 2.)
                    + tutorial.gouy_chapman_density(tutorial.box_l_z - tutorial.HS_ION_SIZE - tutorial.zs, tutorial.CONCENTRATION, tutorial.DEBYE_LENGTH, tutorial.POTENTIAL_DIFF / 2.)) / 2.
        msg = 'The density profile is not sufficiently equal to PB theory.'
        np.testing.assert_allclose(
            charge_profile,
            analytic,
            rtol=5e-2,
            atol=5e-2,
            err_msg=msg)

    def test_capacitance(self):
        # For low potentials the capacitance should be in line with Graham/DH
        # equilibration limiting (2.5 minutes total)
        graham = -tutorial.sigma_vs_phi[:, 0] / (
            constants.elementary_charge / (constants.Boltzmann * tutorial.TEMPERATURE))
        msg = 'The capacitance at low potentials should be in line with Graham/DH.'
        np.testing.assert_allclose(
            graham, tutorial.sigma_vs_phi[:, 1], atol=.015, err_msg=msg)


if __name__ == "__main__":
    ut.main()
