/*
  Copyright (C) 2010-2018 The ESPResSo project
  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010
  Max-Planck-Institute for Polymer Research, Theory Group

  This file is part of ESPResSo.

  ESPResSo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ESPResSo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/** \file
 *  Pressure calculation. Really similar to energy.hpp.
 */

#ifndef CORE_PRESSURE_INLINE_HPP
#define CORE_PRESSURE_INLINE_HPP

#include "debug.hpp"
#include "forces_inline.hpp"
#include "integrate.hpp"
#include "npt.hpp"
#include "pressure.hpp"
#include "thermostat.hpp"
#include "utils.hpp"

#include "electrostatics_magnetostatics/coulomb.hpp"

/** Calculate non bonded energies between a pair of particles.
 *  @param p1        pointer to particle 1.
 *  @param p2        pointer to particle 2.
 *  @param d         vector between p1 and p2.
 *  @param dist      distance between p1 and p2.
 *  @param dist2     distance squared between p1 and p2.
 */
inline void add_non_bonded_pair_virials(Particle *p1, Particle *p2, double d[3],
                                        double dist, double dist2) {
  int p1molid, p2molid, k, l;
  double force[3] = {0, 0, 0};

#ifdef EXCLUSIONS
  if (do_nonbonded(p1, p2))
#endif
  {
    calc_non_bonded_pair_force(p1, p2, d, dist, dist2, force);
    *obsstat_nonbonded(&virials, p1->p.type, p2->p.type) +=
        d[0] * force[0] + d[1] * force[1] + d[2] * force[2];

    /* stress tensor part */
    for (k = 0; k < 3; k++)
      for (l = 0; l < 3; l++)
        obsstat_nonbonded(&p_tensor, p1->p.type, p2->p.type)[k * 3 + l] +=
            force[k] * d[l];

    p1molid = p1->p.mol_id;
    p2molid = p2->p.mol_id;
    if (p1molid == p2molid) {
      *obsstat_nonbonded_intra(&virials_non_bonded, p1->p.type, p2->p.type) +=
          d[0] * force[0] + d[1] * force[1] + d[2] * force[2];

      for (k = 0; k < 3; k++)
        for (l = 0; l < 3; l++)
          obsstat_nonbonded_intra(&p_tensor_non_bonded, p1->p.type,
                                  p2->p.type)[k * 3 + l] += force[k] * d[l];
    }
    if (p1molid != p2molid) {
      *obsstat_nonbonded_inter(&virials_non_bonded, p1->p.type, p2->p.type) +=
          d[0] * force[0] + d[1] * force[1] + d[2] * force[2];

      for (k = 0; k < 3; k++)
        for (l = 0; l < 3; l++)
          obsstat_nonbonded_inter(&p_tensor_non_bonded, p1->p.type,
                                  p2->p.type)[k * 3 + l] += force[k] * d[l];
    }
  }

#ifdef ELECTROSTATICS
  /* real space Coulomb */
  if (coulomb.method != COULOMB_NONE) {
    Coulomb::add_pair_pressure(p1, p2, d, dist, dist2, virials, p_tensor);
  }
#endif /*ifdef ELECTROSTATICS */

#ifdef DIPOLES
  /* real space magnetic dipole-dipole */
  if (coulomb.Dmethod != DIPOLAR_NONE) {
    fprintf(stderr, "calculating pressure for magnetostatics which doesn't "
                    "have it implemented\n");
  }
#endif /*ifdef DIPOLES */
}

inline void calc_bonded_force(Particle *p1, Particle *p2,
                              Bonded_ia_parameters *iaparams, int *i,
                              double dx[3], double force[3]) {
#ifdef TABULATED
// char* errtxt;
#endif

  /* Calculates the bonded force between two particles */
  switch (iaparams->type) {
  case BONDED_IA_FENE:
    calc_fene_pair_force(p1, p2, iaparams, dx, force);
    break;
#ifdef ROTATION
  case BONDED_IA_HARMONIC_DUMBBELL:
    calc_harmonic_dumbbell_pair_force(p1, p2, iaparams, dx, force);
    break;
#endif
  case BONDED_IA_HARMONIC:
    calc_harmonic_pair_force(p1, p2, iaparams, dx, force);
    break;
#ifdef LENNARD_JONES
  case BONDED_IA_SUBT_LJ:
    calc_subt_lj_pair_force(p1, p2, iaparams, dx, force);
    break;
#endif
    /* since it is not clear at the moment how to handle a many body interaction
     * here, I skip it */
  case BONDED_IA_ANGLE_HARMONIC:
    (*i)++;
    force[0] = force[1] = force[2] = 0;
    break;
  case BONDED_IA_ANGLE_COSINE:
    (*i)++;
    force[0] = force[1] = force[2] = 0;
    break;
  case BONDED_IA_ANGLE_COSSQUARE:
    (*i)++;
    force[0] = force[1] = force[2] = 0;
    break;
  case BONDED_IA_DIHEDRAL:
    (*i) += 2;
    force[0] = force[1] = force[2] = 0;
    break;

#ifdef TABULATED
  case BONDED_IA_TABULATED:
    // printf("BONDED TAB, Particle: %d, P2: %d TYPE_TAB:
    // %d\n",p1->p.identity,p2->p.identity,iparams->p.tab.type);
    switch (iaparams->p.tab.type) {
    case 1:
      calc_tab_bond_force(p1, p2, iaparams, dx, force);
      break;
    case 2:
      (*i)++;
      force[0] = force[1] = force[2] = 0;
      break;
    case 3:
      (*i) += 2;
      force[0] = force[1] = force[2] = 0;
      break;
    default:
      runtimeErrorMsg() << "calc_bonded_force: tabulated bond type of atom "
                        << p1->p.identity << " unknown\n";
      return;
    }
    break;
#endif
#ifdef BOND_CONSTRAINT
  case BONDED_IA_RIGID_BOND:
    force[0] = force[1] = force[2] = 0;
    break;
#endif
  case BONDED_IA_VIRTUAL_BOND:
    force[0] = force[1] = force[2] = 0;
    break;
  default:
    //      fprintf(stderr,"add_bonded_virials: WARNING: Bond type %d of atom %d
    //      unhandled\n",bonded_ia_params[type_num].type,p1->p.identity);
    fprintf(stderr,
            "add_bonded_virials: WARNING: Bond type %d , atom %d unhandled, "
            "Atom 2: %d\n",
            iaparams->type, p1->p.identity, p2->p.identity);
    force[0] = force[1] = force[2] = 0;
    break;
  }
}

/* calc_three_body_bonded_forces is called by add_three_body_bonded_stress. This
   routine is only entered for angular potentials. */
inline void calc_three_body_bonded_forces(Particle *p1, Particle *p2,
                                          Particle *p3,
                                          Bonded_ia_parameters *iaparams,
                                          double force1[3], double force2[3],
                                          double force3[3]) {

#ifdef TABULATED
// char* errtxt;
#endif

  switch (iaparams->type) {
#ifdef BOND_ANGLE_OLD
  case BONDED_IA_ANGLE_OLD:
    // p1 is *p_mid, p2 is *p_left, p3 is *p_right
    calc_angle_3body_forces(p1, p2, p3, iaparams, force1, force2, force3);
    break;
#endif
#ifdef BOND_ANGLE
  case BONDED_IA_ANGLE_HARMONIC:
    // p1 is *p_mid, p2 is *p_left, p3 is *p_right
    calc_angle_harmonic_3body_forces(p1, p2, p3, iaparams, force1, force2,
                                     force3);
    break;
  case BONDED_IA_ANGLE_COSINE:
    // p1 is *p_mid, p2 is *p_left, p3 is *p_right
    calc_angle_cosine_3body_forces(p1, p2, p3, iaparams, force1, force2,
                                   force3);
    break;
  case BONDED_IA_ANGLE_COSSQUARE:
    // p1 is *p_mid, p2 is *p_left, p3 is *p_right
    calc_angle_cossquare_3body_forces(p1, p2, p3, iaparams, force1, force2,
                                      force3);
    break;
#endif
#ifdef TABULATED
  case BONDED_IA_TABULATED:
    switch (iaparams->p.tab.type) {
    case TAB_BOND_ANGLE:
      // p1 is *p_mid, p2 is *p_left, p3 is *p_right
      calc_angle_3body_tabulated_forces(p1, p2, p3, iaparams, force1, force2,
                                        force3);
      break;
    default:
      runtimeErrorMsg() << "calc_bonded_force: tabulated bond type of atom "
                        << p1->p.identity << " unknown\n";
      return;
    }
    break;
#endif
  default:
    fprintf(stderr, "calc_three_body_bonded_forces: \
            WARNING: Bond type %d , atom %d unhandled, Atom 2: %d\n",
            iaparams->type, p1->p.identity, p2->p.identity);
    break;
  }
}

/** Calculate bonded virials for one particle.
 *  For performance reasons the force routines add their values directly to the
 *  particles. So here we do some tricks to get the value out without changing
 *  the forces.
 *  @param p1 particle for which to calculate virials
 */
inline void add_bonded_virials(Particle *p1) {
  double force[3] = {0, 0, 0};
  // char *errtxt;
  Particle *p2;
  Bonded_ia_parameters *iaparams;

  int i, k, l;
  int type_num;

  i = 0;
  while (i < p1->bl.n) {
    type_num = p1->bl.e[i++];
    iaparams = &bonded_ia_params[type_num];

    /* fetch particle 2 */
    p2 = local_particles[p1->bl.e[i++]];
    if (!p2) {
      // for harmonic spring:
      // if cutoff was defined and p2 is not there it is anyway outside the
      // cutoff, see calc_maximal_cutoff()
      if ((type_num == BONDED_IA_HARMONIC) && (iaparams->p.harmonic.r_cut > 0))
        return;
      runtimeErrorMsg() << "bond broken between particles " << p1->p.identity
                        << " and " << p1->bl.e[i - 1]
                        << " (particles not stored on the same node)";
      return;
    }

    double a[3] = {p1->r.p[0], p1->r.p[1], p1->r.p[2]};
    double b[3] = {p2->r.p[0], p2->r.p[1], p2->r.p[2]};
    auto dx = get_mi_vector(a, b);
    calc_bonded_force(p1, p2, iaparams, &i, dx.data(), force);
    *obsstat_bonded(&virials, type_num) +=
        dx[0] * force[0] + dx[1] * force[1] + dx[2] * force[2];

    /* stress tensor part */
    for (k = 0; k < 3; k++)
      for (l = 0; l < 3; l++)
        obsstat_bonded(&p_tensor, type_num)[k * 3 + l] += force[k] * dx[l];
  }
}

/** Calculate the contribution to the stress tensor from angular potentials.
 *  The central particle of the three-particle interaction is responsible
 *  for the contribution of the entire interaction - this is the coding
 *  not the physics.
 */
inline void add_three_body_bonded_stress(Particle *p1) {
  double dx12[3]; // espresso notation
  double dx21[3];
  double dx31[3];
  double force1[3];
  double force2[3];
  double force3[3];

  // char *errtxt;
  Particle *p2;
  Particle *p3;
  Bonded_ia_parameters *iaparams;

  int i, k, j, l;
  int type_num;
  BondedInteraction type;

  i = 0;
  while (i < p1->bl.n) {
    /* scan bond list for angular interactions */
    type_num = p1->bl.e[i];
    iaparams = &bonded_ia_params[type_num];
    type = iaparams->type;

    if (type == BONDED_IA_ANGLE_HARMONIC || type == BONDED_IA_ANGLE_COSINE ||
        type == BONDED_IA_ANGLE_COSSQUARE) {
      p2 = local_particles[p1->bl.e[++i]];
      p3 = local_particles[p1->bl.e[++i]];

      get_mi_vector(dx12, p1->r.p, p2->r.p);
      for (j = 0; j < 3; j++)
        dx21[j] = -dx12[j];

      get_mi_vector(dx31, p3->r.p, p1->r.p);

      for (j = 0; j < 3; j++) {
        force1[j] = 0.0;
        force2[j] = 0.0;
        force3[j] = 0.0;
      }

      calc_three_body_bonded_forces(p1, p2, p3, iaparams, force1, force2,
                                    force3);

      /* uncomment the next line to see that the virial is indeed zero */
      // printf("W = %g\n", scalar(force2, dx21) + scalar(force3, dx31));

      /* three-body bonded interactions contribute to the stress but not the
       * scalar pressure */
      for (k = 0; k < 3; k++) {
        for (l = 0; l < 3; l++) {
          obsstat_bonded(&p_tensor, type_num)[3 * k + l] +=
              force2[k] * dx21[l] + force3[k] * dx31[l];
        }
      }
      i = i + 1;
    }
    // skip over non-angular interactions
    else if (type == BONDED_IA_FENE) {
      i = i + 2;
    } else if (type == BONDED_IA_OIF_GLOBAL_FORCES) {
      i = i + 3;
    } else if (type == BONDED_IA_OIF_LOCAL_FORCES) {
      i = i + 4;
    } else if (type == BONDED_IA_OIF_OUT_DIRECTION) {
      i = i + 3;
    } else if (type == BONDED_IA_HARMONIC) {
      i = i + 2;
    }
#ifdef LENNARD_JONES
    else if (type == BONDED_IA_SUBT_LJ) {
      i = i + 2;
    }
#endif
    else if (type == BONDED_IA_DIHEDRAL) {
      i = i + 4;
    }
#ifdef TABULATED
    else if (type == BONDED_IA_TABULATED) {
      if (iaparams->p.tab.type == TAB_BOND_LENGTH) {
        i = i + 2;
      } else if (iaparams->p.tab.type == TAB_BOND_ANGLE) {
        p2 = local_particles[p1->bl.e[++i]];
        p3 = local_particles[p1->bl.e[++i]];

        get_mi_vector(dx12, p1->r.p, p2->r.p);
        for (j = 0; j < 3; j++)
          dx21[j] = -dx12[j];

        get_mi_vector(dx31, p3->r.p, p1->r.p);

        for (j = 0; j < 3; j++) {
          force1[j] = 0.0;
          force2[j] = 0.0;
          force3[j] = 0.0;
        }

        calc_three_body_bonded_forces(p1, p2, p3, iaparams, force1, force2,
                                      force3);

        /* uncomment the next line to see that the virial is indeed zero */
        // printf("W = %g\n", scalar(force2, dx21) + scalar(force3, dx31));

        /* three-body bonded interactions contribute to the stress but not the
         * scalar pressure */
        for (k = 0; k < 3; k++) {
          for (l = 0; l < 3; l++) {
            obsstat_bonded(&p_tensor, type_num)[3 * k + l] +=
                force2[k] * dx21[l] + force3[k] * dx31[l];
          }
        }
        i = i + 1;
      } else if (iaparams->p.tab.type == TAB_BOND_DIHEDRAL) {
        i = i + 4;
      } else {
        runtimeErrorMsg()
            << "add_three_body_bonded_stress: match not found for particle "
            << p1->p.identity << ".\n";
      }
    }
#endif
#ifdef BOND_CONSTRAINT
    else if (type == BONDED_IA_RIGID_BOND) {
      i = i + 2;
    }
#endif
    else if (type == BONDED_IA_VIRTUAL_BOND) {
      i = i + 2;
    } else {
      runtimeErrorMsg()
          << "add_three_body_bonded_stress: match not found for particle "
          << p1->p.identity << ".\n";
    }
  }
}

/** Calculate kinetic pressure (aka energy) for one particle.
 *  @param p1 particle for which to calculate pressure
 *  @param v_comp flag which enables (1) compensation of the velocities required
 *                for deriving a pressure reflecting \ref nptiso_struct::p_inst
 *                (hence it only works with domain decomposition); naturally it
 *                therefore doesn't make sense to use it without NpT.
 */
inline void add_kinetic_virials(Particle *p1, int v_comp) {
  int k, l;
  /* kinetic energy */
  {
    if (v_comp)
      virials.data.e[0] +=
          (Utils::sqr(p1->m.v[0] * time_step -
                      p1->f.f[0] * 0.5 * time_step * time_step / p1->p.mass) +
           Utils::sqr(p1->m.v[1] * time_step -
                      p1->f.f[1] * 0.5 * time_step * time_step / p1->p.mass) +
           Utils::sqr(p1->m.v[2] * time_step -
                      p1->f.f[2] * 0.5 * time_step * time_step / p1->p.mass)) *
          (*p1).p.mass;
    else
      virials.data.e[0] += (Utils::sqr(p1->m.v[0] * time_step) +
                            Utils::sqr(p1->m.v[1] * time_step) +
                            Utils::sqr(p1->m.v[2] * time_step)) *
                           (*p1).p.mass;
  }

  /* ideal gas contribution (the rescaling of the velocities by '/=time_step'
   * each will be done later) */
  for (k = 0; k < 3; k++)
    for (l = 0; l < 3; l++)
      p_tensor.data.e[k * 3 + l] +=
          (p1->m.v[k] * time_step) * (p1->m.v[l] * time_step) * (*p1).p.mass;
}

#endif
