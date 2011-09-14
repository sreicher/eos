/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2010, 2011 Danny van Dyk
 * Copyright (c) 2010 Christian Wacker
 *
 * This file is part of the EOS project. EOS is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * EOS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <eos/rare-b-decays/charm-loops.hh>
#include <eos/rare-b-decays/exclusive-b-to-s-dilepton-low-recoil.hh>
#include <eos/rare-b-decays/form-factors.hh>
#include <eos/rare-b-decays/hard-scattering.hh>
#include <eos/rare-b-decays/long-distance.hh>
#include <eos/utils/destringify.hh>
#include <eos/utils/integrate.hh>
#include <eos/utils/kinematic.hh>
#include <eos/utils/memoise.hh>
#include <eos/utils/model.hh>
#include <eos/utils/options.hh>
#include <eos/utils/power_of.hh>
#include <eos/utils/private_implementation_pattern-impl.hh>
#include <eos/utils/qcd.hh>
#include <eos/utils/save.hh>

#include <cmath>
#include <functional>

namespace eos
{
    using std::norm;

    struct ShortDistanceLowRecoil
    {
        /*!
         * Effective Wilson coefficient c7 in the region of low hadronic recoil.
         *
         * @param s             dilepton invariant mass
         * @param mu            renormalization scale
         * @param alpha_s       strong coupling evaluated at the scale mu
         * @param m_b_PS        PS mass of the bottom quark
         * @param use_nlo       true, if NLO contributions shall be used
         * @param wc            the Wilson coefficients
         *
         * For the calculation, cf. [GP2004], Eq. (56)
         */
        static complex<double> c7eff(const double & s, const double & mu, const double & alpha_s, const double & m_b_PS, bool use_nlo,
                const WilsonCoefficients<BToS> & wc)
        {
            // cf. [BFS2001] Eq. (29), p. 8, and Eqs. (82)-(84), p. 30
            complex<double> lo = -1.0/3.0 * wc.c3() - 4.0/9.0 * wc.c4() - 20.0/3.0 * wc.c5() - 80.0/9.0 * wc.c6();
            complex<double> nlo = -1.0 * (
                      wc.c1() * CharmLoops::F17_massless(mu, s, m_b_PS)
                    + wc.c2() * CharmLoops::F27_massless(mu, s, m_b_PS)
                    + wc.c8() * CharmLoops::F87_massless(mu, s, m_b_PS));

            complex<double> result = wc.c7() + lo;
            if (use_nlo)
                result += (alpha_s / (4.0 * M_PI)) * nlo;

            return result;
        }

        /*!
         * Effective Wilson coefficient c9 in the region of low hadronic recoil.
         *
         * @param s                     dilepton invariant mass
         * @param mu                    renormalization scale
         * @param alpha_s               strong coupling evaluated at the scale mu
         * @param m_b_PS                PS mass of the bottom quark
         * @param m_c                   MSbar mass of the charm quark
         * @param use_nlo               true, if NLO contributions shall be used
         * @param ccbar_resonance       true, if phenomenological data from e^+e^- -> ccbar resonance -> hadrons shall be used
         * @param lambda_hat_u          certain combination of CKM matrix elements: V_ub V_us^* / (V_tb V_ts^*)
         * @param wc                    the Wilson coefficients
         *
         * For the calculation, cf. [GP2004], Eq. (55), p. 10
         */
        static complex<double> c9eff(const double & s, const double & mu, const double & alpha_s, const double & m_b_PS, const double & m_c_MSbar,
                bool use_nlo, bool ccbar_resonance, const complex<double> & lambda_hat_u,
                const WilsonCoefficients<BToS> & wc)
        {
            // Uses b pole mass according to [BFS2001], Sec. 3.1, paragraph Quark Masses
            // Substitute pole mass by PS mass
            complex<double> c = -2.0 / 27.0 * (8.0 * wc.c1() + 6.0 * wc.c2() - 6.0 * wc.c3() - 8.0 * wc.c4() - 12.0 * wc.c5() - 160.0 * wc.c6());
            complex<double> c_0 = -2.0 / 27.0 * (48.0 * wc.c1() + 36.0 * wc.c2() + 198.0 * wc.c3() - 24.0 * wc.c4() + 1872.0 * wc.c5() - 384.0 * wc.c6());
            complex<double> c_b = +2.0 / 27.0 * (126.0 * wc.c3() + 24.0 * wc.c4() + 1368.0 * wc.c5() + 384.0 * wc.c6());
            complex<double> G0 = -3.0 / 8.0 * ((ccbar_resonance ? LongDistance::g_had_ccbar(s, m_c_MSbar) : CharmLoops::h(mu, s)) + 4.0 / 9.0);
            complex<double> Gb = -3.0 / 8.0 * (CharmLoops::h(mu, s, m_b_PS) + 4.0 / 9.0);

            complex<double> lo = c_b * Gb + c_0 * G0 + c;
            complex<double> nlo_alpha_s = -1.0 * (wc.c1() * CharmLoops::F19_massless(mu, s, m_b_PS)
                                                + wc.c2() * CharmLoops::F29_massless(mu, s, m_b_PS)
                                                + wc.c8() * CharmLoops::F89_massless(s, m_b_PS));
            complex<double> nlo_mc = m_c_MSbar * m_c_MSbar / s * 8.0 *
                ((4.0/9.0 * wc.c1() + 1.0/3.0 * wc.c2()) * (1.0 + lambda_hat_u) + 2.0 * wc.c3() + 20.0 * wc.c5());

            complex<double> result = wc.c9() + lo;
            if ((! ccbar_resonance) && (use_nlo))
                result += (alpha_s / (4.0 * M_PI)) * nlo_alpha_s + nlo_mc;

            return result;
        }
    };

    /*
     * Decay: B -> K^* l lbar at Low Recoil, cf. [BHvD2010]
     */
    template <>
    struct Implementation<BToKstarDilepton<LowRecoil>>
    {
        std::shared_ptr<Model> model;

        UsedParameter hbar;

        UsedParameter m_b_MSbar;

        UsedParameter m_c_MSbar;

        UsedParameter m_s;

        UsedParameter m_B;

        UsedParameter m_Kstar;

        UsedParameter m_l;

        UsedParameter mu;

        UsedParameter alpha_e;

        UsedParameter g_fermi;

        UsedParameter lambda_long;

        UsedParameter lambda_par;

        UsedParameter lambda_perp;

        UsedParameter sl_phase_long;

        UsedParameter sl_phase_par;

        UsedParameter sl_phase_perp;

        UsedParameter tau;

        std::shared_ptr<FormFactors<PToV>> form_factors;

        bool cp_conjugate;

        bool ccbar_resonance;

        bool use_nlo;

        Implementation(const Parameters & p, const Options & o, ParameterUser & u) :
            model(Model::make(o.get("model", "SM"), p, o)),
            hbar(p["hbar"], u),
            m_b_MSbar(p["mass::b(MSbar)"], u),
            m_c_MSbar(p["mass::c"], u),
            m_s(p["mass::s"], u),
            m_B(p["mass::B_" + o.get("q", "d")], u),
            m_Kstar(p["mass::K^*0"], u),
            m_l(p["mass::" + o.get("l", "mu")], u),
            mu(p["mu"], u),
            alpha_e(p["QED::alpha_e(m_b)"], u),
            g_fermi(p["G_Fermi"], u),
            lambda_long(p["B->Vll::Lambda_0@LowRecoil"], u),
            lambda_par(p["B->Vll::Lambda_pa@LowRecoil"], u),
            lambda_perp(p["B->Vll::Lambda_pp@LowRecoil"], u),
            sl_phase_long(p["B->Vll::sl_phase_0@LowRecoil"], u),
            sl_phase_par(p["B->Vll::sl_phase_pa@LowRecoil"], u),
            sl_phase_perp(p["B->Vll::sl_phase_pp@LowRecoil"], u),
            tau(p["life_time::B_" + o.get("q", "d")], u),
            cp_conjugate(destringify<bool>(o.get("cp-conjugate", "false"))),
            ccbar_resonance(destringify<bool>(o.get("ccbar-resonance", "false"))),
            use_nlo(destringify<bool>(o.get("nlo", "true")))
        {
            form_factors = FormFactorFactory<PToV>::create("B->K^*@" + o.get("form-factors", "KMPW2010"), p);

            std::string spectator_quark = o.get("q", "d");
            if ((spectator_quark != "d") && (spectator_quark != "u"))
                throw InternalError("Unsupported spectator quark");

            if (! form_factors.get())
                throw InternalError("Form factors not found!");

            u.uses(*form_factors);
            u.uses(*model);
        }

        // We use the PS mass except for kappa
        double m_b_PS() const
        {
            // Actually use m_b_PS at mu_PS = 2.0 GeV
            return model->m_b_ps(2.0);
        }

        // cf. [GP2004], Eq. (56)
        complex<double> c7eff(const WilsonCoefficients<BToS> & wc, double s) const
        {
            return ShortDistanceLowRecoil::c7eff(s, mu(), model->alpha_s(mu), m_b_PS(), use_nlo, wc);
        }

        // cf. [GP2004], Eq. (55), p. 10
        complex<double> c9eff(const WilsonCoefficients<BToS> & wc, const double & s) const
        {
            complex<double> lambda_hat_u = (model->ckm_ub() * conj(model->ckm_us())) / (model->ckm_tb() * conj(model->ckm_ts()));
            if (cp_conjugate)
            {
                lambda_hat_u = conj(lambda_hat_u);
            }

            return ShortDistanceLowRecoil::c9eff(s, mu(), model->alpha_s(mu), m_b_PS(), model->m_c_msbar(mu), use_nlo, ccbar_resonance, lambda_hat_u, wc);
        }

        double rho_1(const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);

            return std::norm(c9eff(wc, s) + kappa() * (2.0 * m_b_MSbar * m_B / s) * c7eff(wc, s)) + std::norm(wc.c10());
        }

        double rho_2(const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);

            return real((c9eff(wc, s) + kappa() * (2.0 * m_b_MSbar * m_B / s) * c7eff(wc, s)) * conj(wc.c10()));
        }

        complex<double> rho_L(const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);

            return c9eff(wc, s) + kappa() * (2.0 * m_b_MSbar * m_B / s) * c7eff(wc, s) - wc.c10();
        }

        complex<double> rho_R(const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);

            return c9eff(wc, s) + kappa() * (2.0 * m_b_MSbar * m_B / s) * c7eff(wc, s) + wc.c10();
        }

        double kappa() const
        {
            // cf. [BHvD2010], Eq. (3.8), p. 8
            // Use m_b_MSbar(m_b_MSbar) instead m_b_MSbar(mu), as we want kappa up to NLO only.
            return (1.0 - 2.0 * model->alpha_s(mu) / (3.0 * M_PI) * std::log(mu / m_b_MSbar));
        }

        double norm(const double & s) const
        {
            double lambda_t = abs(model->ckm_tb() * conj(model->ckm_ts()));

            return std::sqrt(power_of<2>(g_fermi() * alpha_e()) / 3.0 / 1024 / std::pow(M_PI, 5.0) / m_B
                    * lambda_t * lambda_t * s_hat(s)
                    * std::sqrt(lambda(m_B * m_B, m_Kstar * m_Kstar, s))); // cf. [BHP2008], Eq. (C.6), p. 21
        }

        inline double s_hat(const double & s) const
        {
            return s / m_B / m_B;
        }

        // Amplitudes
        complex<double> a_long(const Helicity & helicity, const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);
            double h = helicity;
            double m_Kstarhat = m_Kstar / m_B;
            double m_Kstarhat2 = std::pow(m_Kstarhat, 2);
            double s_hat = s / m_B / m_B;
            double a_1 = form_factors->a_1(s), a_2 = form_factors->a_2(s);
            double alpha_s = model->alpha_s(mu());

            complex<double> prefactor = complex<double>(-1.0, 0.0) * m_B()
                / (2.0 * m_Kstarhat * (1.0 + m_Kstarhat) * std::sqrt(s_hat));
            complex<double> wilson1 = (c9eff(wc, s) - wc.c9prime()) + h * (wc.c10() - wc.c10prime())
                + kappa() * (c7eff(wc, s) - wc.c7prime()) * (2 * m_B / s) * (m_b_MSbar() - m_s() - lambda_par())
                + 0.5 * lambda_par() / m_B * alpha_s * complex<double>(std::cos(sl_phase_par()), std::sin(sl_phase_par()));
            complex<double> wilson2 = (c9eff(wc, s) - wc.c9prime()) + h * (wc.c10() - wc.c10prime())
                + kappa() * (c7eff(wc, s) - wc.c7prime()) * (2 * m_B / s) * (m_b_MSbar() - m_s() - lambda_long())
                - 0.5 * lambda_long / m_B * alpha_s * complex<double>(std::cos(sl_phase_long()), std::sin(sl_phase_long()));
            double formfactor1 = (1.0 - m_Kstarhat2 - s_hat) * std::pow(1.0 + m_Kstarhat, 2) * a_1,
                   formfactor2 = -lambda(1.0, m_Kstarhat2, s_hat) * a_2;

            return this->norm(s) * prefactor * (wilson1 * formfactor1 + wilson2 * formfactor2); // cf. [BHvD2010], Eq. (3.15), p. 10
        }

        complex<double> a_perp(const Helicity & helicity, const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);
            double h = helicity;
            double m_Kstarhat = m_Kstar / m_B;
            double m_Kstarhat2 = std::pow(m_Kstarhat, 2);
            double alpha_s = model->alpha_s(mu());

            complex<double> prefactor = complex<double>(1.0, 0.0) * m_B();
            complex<double> wilson = ((c9eff(wc, s) + wc.c9prime()) + h * (wc.c10() + wc.c10prime()))
                + kappa() * (c7eff(wc, s) + wc.c7prime()) * (2 * m_B / s) * (m_b_MSbar() + m_s() + lambda_perp())
                - 0.5 * lambda_perp()/m_B * alpha_s * complex<double>(std::cos(sl_phase_perp()), std::sin(sl_phase_perp()));
            double formfactor = std::sqrt(2 * lambda(1.0, m_Kstarhat2, s_hat(s))) / (1.0 + m_Kstarhat)
                * form_factors->v(s);

            return this->norm(s) * prefactor * wilson * formfactor; // cf. [BHvD2010], Eq. (3.13), p. 10
        }

        complex<double> a_par(const Helicity & helicity, const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);
            double h = helicity;
            double m_Kstarhat = m_Kstar / m_B;
            double alpha_s = model->alpha_s(mu());

            complex<double> prefactor = complex<double>(-1.0, 0.0) * m_B();
            complex<double> wilson = ((c9eff(wc, s) - wc.c9prime()) + h * (wc.c10() - wc.c10prime()))
                + kappa() * (c7eff(wc, s) - wc.c7prime()) * (2 * m_B / s) * (m_b_MSbar() - m_s() - lambda_par())
                + 0.5 * lambda_par() * alpha_s * complex<double>(std::cos(sl_phase_par()), std::sin(sl_phase_par()));
            complex<double> formfactor = std::sqrt(2) * (1.0 + m_Kstarhat) * form_factors->a_1(s);

            return this->norm(s) * prefactor * wilson * formfactor; // cf. [BHvD2010], Eq. (3.14), p. 10
        }

        complex<double> a_timelike(const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);
            double m_Kstarhat = m_Kstar / m_B;
            double shat = s_hat(s);

            return this->norm(s) * m_B * sqrt(lambda(1.0, power_of<2>(m_Kstarhat), shat) / shat) *
                complex<double>(0.0, 2.0) * (wc.c10() - wc.c10prime()) * form_factors->a_0(s);
        }

        // Quantity Y = Y_9 + lambda_u_hat Y_9^u + kappa_hat Y_7, the strong phase contributor of the amplitudes
        complex<double> Y(const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);

            return (c9eff(wc, s) - wc.c9()) + kappa() * (c7eff(wc, s) - wc.c7()) * (2.0 * m_b_MSbar * m_B / s);
        }

        // Unormalized combinations of transversity amplitudes
        double u_1(const double & s) const
        {
            return std::norm(a_long(left_handed, s)) + std::norm(a_long(right_handed, s));
        }

        double u_2(const double & s) const
        {
            return std::norm(a_perp(left_handed, s)) + std::norm(a_perp(right_handed, s));
        }

        double u_3(const double & s) const
        {
            return std::norm(a_par(left_handed, s)) + std::norm(a_par(right_handed, s));
        }

        double u_4(const double & s) const
        {
            return real(a_long(left_handed, s) * conj(a_par(left_handed, s)) + conj(a_long(right_handed, s)) * a_par(right_handed, s));
        }

        double u_5(const double & s) const
        {
            return (real(a_long(left_handed, s) * conj(a_perp(left_handed, s)) - conj(a_long(right_handed, s)) * a_perp(right_handed, s)));
        }

        double u_6(const double & s) const
        {
            return (real(a_par(left_handed, s) * conj(a_perp(left_handed, s)) - conj(a_par(right_handed, s)) * a_perp(right_handed, s)));
        }

        double u_7(const double & s) const
        {
            return imag(a_long(left_handed, s) * conj(a_par(left_handed, s)) + conj(a_long(right_handed, s)) * a_par(right_handed, s));
        }

        double u_8(const double & s) const
        {
            return (imag(a_long(left_handed, s) * conj(a_perp(left_handed, s)) - conj(a_long(right_handed, s)) * a_perp(right_handed, s)));
        }

        double u_9(const double & s) const
        {
            return (imag(a_par(left_handed, s) * conj(a_perp(left_handed, s)) - conj(a_par(right_handed, s)) * a_perp(right_handed, s)));
        }

        // Components of observables
        double decay_width(const double & s) const
        {
            return (u_1(s) + u_2(s) + u_3(s));
        }

        double differential_branching_ratio(const double & s) const
        {
            return decay_width(s) * tau() / hbar();
        }

        double a_fb_numerator(const double & s) const
        {
            return 1.5 * (real(a_par(left_handed, s) * conj(a_perp(left_handed, s))) - real(a_par(right_handed, s) * conj(a_perp(right_handed, s))));
        }

        double f_l_numerator(const double & s) const
        {
            return u_1(s);
        }

        double a_t_2_numerator(const double & s) const
        {
            return (u_2(s) - u_3(s));
        }

        double a_t_2_denominator(const double & s) const
        {
            return (u_2(s) + u_3(s));
        }

        double a_t_3_numerator(const double & s) const
        {
            return sqrt(pow(u_4(s), 2) + pow(u_7(s), 2));
        }

        double a_t_3_denominator(const double & s) const
        {
            return sqrt(u_1(s) * u_2(s));
        }

        double a_t_4_numerator(const double & s) const
        {
            return sqrt(pow(u_5(s), 2) + pow(u_8(s), 2));
        }

        double a_t_4_denominator(const double & s) const
        {
            return u_4(s);
        }

        double h_1_numerator(const double & s) const
        {
            return u_4(s);
        }

        double h_1_denominator(const double & s) const
        {
            return sqrt(u_1(s) * u_3(s));
        }

        double h_2_numerator(const double & s) const
        {
            return u_5(s);
        }

        double h_2_denominator(const double & s) const
        {
            return sqrt(u_1(s) * u_2(s));
        }

        double h_3_numerator(const double & s) const
        {
            return u_6(s);
        }

        double h_3_denominator(const double & s) const
        {
            return sqrt(u_2(s) * u_3(s));
        }
    };

    BToKstarDilepton<LowRecoil>::BToKstarDilepton(const Parameters & parameters, const Options & options) :
        PrivateImplementationPattern<BToKstarDilepton<LowRecoil>>(new Implementation<BToKstarDilepton<LowRecoil>>(parameters, options, *this))
    {
    }

    BToKstarDilepton<LowRecoil>::~BToKstarDilepton()
    {
    }

    complex<double>
    BToKstarDilepton<LowRecoil>::a_long(const Helicity & h, const double & s) const
    {
        return _imp->a_long(h, s);
    }

    complex<double>
    BToKstarDilepton<LowRecoil>::a_perp(const Helicity & h, const double & s) const
    {
        return _imp->a_perp(h, s);
    }

    complex<double>
    BToKstarDilepton<LowRecoil>::a_par(const Helicity & h, const double & s) const
    {
        return _imp->a_par(h, s);
    }

    double
    BToKstarDilepton<LowRecoil>::real_y(const double & s) const
    {
        return real(_imp->Y(s));
    }

    double
    BToKstarDilepton<LowRecoil>::imag_y(const double & s) const
    {
        return imag(_imp->Y(s));
    }

    double
    BToKstarDilepton<LowRecoil>::real_c9eff(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return real(_imp->c9eff(wc, s));
    }

    double
    BToKstarDilepton<LowRecoil>::imag_c9eff(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return imag(_imp->c9eff(wc, s));
    }

    double
    BToKstarDilepton<LowRecoil>::real_c7eff(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return real(_imp->c7eff(wc, s));
    }

    double
    BToKstarDilepton<LowRecoil>::imag_c7eff(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return imag(_imp->c7eff(wc, s));
    }

    double
    BToKstarDilepton<LowRecoil>::rho_1(const double & s) const
    {
        return _imp->rho_1(s);
    }

    double
    BToKstarDilepton<LowRecoil>::rho_2(const double & s) const
    {
        return _imp->rho_2(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_branching_ratio(const double & s) const
    {
        return _imp->differential_branching_ratio(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_decay_width(const double & s) const
    {
        return _imp->decay_width(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_forward_backward_asymmetry(const double & s) const
    {
        return _imp->a_fb_numerator(s) / _imp->decay_width(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_transverse_asymmetry_2(const double & s) const
    {
        return _imp->a_t_2_numerator(s) / _imp->a_t_2_denominator(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_transverse_asymmetry_3(const double & s) const
    {
        return _imp->a_t_3_numerator(s) / _imp->a_t_3_denominator(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_transverse_asymmetry_4(const double & s) const
    {
        return _imp->a_t_4_numerator(s) / _imp->a_t_4_denominator(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_h_1(const double & s) const
    {
        return _imp->h_1_numerator(s) / _imp->h_1_denominator(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_h_2(const double & s) const
    {
        return _imp->h_2_numerator(s) / _imp->h_2_denominator(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_h_3(const double & s) const
    {
        return _imp->h_3_numerator(s) / _imp->h_3_denominator(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_longitudinal_polarisation(const double & s) const
    {
        return _imp->f_l_numerator(s) / _imp->decay_width(s);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_cp_asymmetry_1(const double & s) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        double rho_1 = _imp->rho_1(s);
        _imp->cp_conjugate = true;
        double rho_1_bar = _imp->rho_1(s);

        return (rho_1 - rho_1_bar) / (rho_1 + rho_1_bar);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_cp_asymmetry_2(const double & s) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        double rho_1 = _imp->rho_1(s), rho_2 = _imp->rho_2(s);
        _imp->cp_conjugate = true;
        double rho_1_bar = _imp->rho_1(s), rho_2_bar = _imp->rho_2(s);

        return (rho_2 / rho_1 - rho_2_bar / rho_1_bar) / (rho_2 / rho_1 + rho_2_bar / rho_1_bar);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_cp_asymmetry_3(const double & s) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        double rho_1 = _imp->rho_1(s), rho_2 = _imp->rho_2(s);
        _imp->cp_conjugate = true;
        double rho_1_bar = _imp->rho_1(s), rho_2_bar = _imp->rho_2(s);

        return 2.0 * (rho_2 - rho_2_bar) / (rho_1 + rho_1_bar);
    }

    double
    BToKstarDilepton<LowRecoil>::differential_cp_asymmetry_mix(const double & s) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        double rho_1 = _imp->rho_1(s), rho_2 = _imp->rho_2(s);

        complex<double> rho_L = _imp->rho_L(s), rho_R = _imp->rho_R(s);
        _imp->cp_conjugate = true;
        complex<double> rho_L_bar = _imp->rho_L(s), rho_R_bar = _imp->rho_R(s);

        double abs2_xi_L = norm(rho_L / rho_L_bar), abs2_xi_R = norm(rho_R / rho_R_bar);

        return (2 * rho_2 * (abs2_xi_L + abs2_xi_R - 2.0) + rho_1 * (abs2_xi_R - abs2_xi_L))
            / (rho_1 * (abs2_xi_L + abs2_xi_R + 2.0) + 2 * rho_2 * (abs2_xi_R - abs2_xi_L));
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_branching_ratio(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> f = std::bind(&BToKstarDilepton<LowRecoil>::differential_branching_ratio,
                this, std::placeholders::_1);

        return integrate(f, 64, s_min, s_max);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_branching_ratio_cp_averaged(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);
        std::function<double (const double &)> f = std::bind(&BToKstarDilepton<LowRecoil>::differential_branching_ratio,
                this, std::placeholders::_1);

        double br = integrate(f, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double br_bar = integrate(f, 64, s_min, s_max);

        return (br + br_bar) / 2.0;
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_forward_backward_asymmetry_naive(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(&BToKstarDilepton<LowRecoil>::differential_forward_backward_asymmetry, this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / (s_max - s_min);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_forward_backward_asymmetry(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::a_fb_numerator, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::decay_width, _imp, std::placeholders::_1);

        return integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_forward_backward_asymmetry_cp_averaged(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);
        std::function<double (const double &)> num = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::a_fb_numerator, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::decay_width, _imp, std::placeholders::_1);

        double a_fb = integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double a_fb_bar = integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);

        return (a_fb + a_fb_bar) / 2.0;
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_unnormalized_forward_backward_asymmetry(const double & s_min, const double & s_max) const
    {
        // Convert from asymmetry in the decay width to asymmetry in the BR
        // cf. [PDG2008] : Gamma = hbar / tau_B, pp. 5, 79
        static const double Gamma(6.58211899e-22 * 1e-3 / 1.53e-12);
        std::function<double (const double &)> integrand = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::a_fb_numerator, _imp, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / Gamma;
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_longitudinal_polarisation(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::f_l_numerator, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::decay_width, _imp, std::placeholders::_1);

        return integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_longitudinal_polarisation_cp_averaged(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);
        std::function<double (const double &)> num = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::f_l_numerator, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::decay_width, _imp, std::placeholders::_1);

        double f_l = integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double f_l_bar = integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);

        return (f_l + f_l_bar) / 2.0;
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_longitudinal_polarisation_naive(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(&BToKstarDilepton<LowRecoil>::differential_longitudinal_polarisation, this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / (s_max - s_min);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_transverse_asymmetry_2(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::a_t_2_numerator, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::a_t_2_denominator, _imp, std::placeholders::_1);

        return integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_transverse_asymmetry_2_cp_averaged(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);
        std::function<double (const double &)> num = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::a_t_2_numerator, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::a_t_2_denominator, _imp, std::placeholders::_1);

        double a_t_2 = integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double a_t_2_bar = integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);

        return (a_t_2 + a_t_2_bar) / 2.0;
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_transverse_asymmetry_2_naive(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(&BToKstarDilepton<LowRecoil>::differential_transverse_asymmetry_2, this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / (s_max - s_min);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_transverse_asymmetry_3(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_4, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_1, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom2 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_2, _imp, std::placeholders::_1);

        return integrate(num1, 64, s_min, s_max)
            / sqrt(integrate(denom1, 64, s_min, s_max) * integrate(denom2, 64, s_min, s_max));
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_transverse_asymmetry_3_naive(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(&BToKstarDilepton<LowRecoil>::differential_transverse_asymmetry_3, this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / (s_max - s_min);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_transverse_asymmetry_4(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num1 = std::bind(
                &Implementation<BToKstarDilepton<LowRecoil>>::u_5, _imp, std::placeholders::_1);

        std::function<double (const double &)> denom1 = std::bind(
                &Implementation<BToKstarDilepton<LowRecoil>>::u_4, _imp, std::placeholders::_1);

        std::function<double (const double &)> denom2 = std::bind(
                &Implementation<BToKstarDilepton<LowRecoil>>::u_7, _imp, std::placeholders::_1);

        return integrate(num1, 64, s_min, s_max)
            / sqrt(pow(integrate(denom1, 64, s_min, s_max), 2) + pow(integrate(denom2, 64, s_min, s_max), 2));
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_transverse_asymmetry_4_naive(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(&BToKstarDilepton<LowRecoil>::differential_transverse_asymmetry_4, this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / (s_max - s_min);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_h_1(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_4, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_1, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom2 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_3, _imp, std::placeholders::_1);

        return integrate(num1, 64, s_min, s_max)
            / sqrt(integrate(denom1, 64, s_min, s_max) * integrate(denom2, 64, s_min, s_max));
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_h_1_naive(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(&BToKstarDilepton<LowRecoil>::differential_h_1, this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / (s_max - s_min);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_h_2(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_5, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_1, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom2 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_2, _imp, std::placeholders::_1);

        return integrate(num1, 64, s_min, s_max)
            / sqrt(integrate(denom1, 64, s_min, s_max) * integrate(denom2, 64, s_min, s_max));
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_h_2_naive(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(&BToKstarDilepton<LowRecoil>::differential_h_2, this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / (s_max - s_min);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_h_3(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_6, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_2, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom2 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_3, _imp, std::placeholders::_1);

        return integrate(num1, 64, s_min, s_max)
            / sqrt(integrate(denom1, 64, s_min, s_max) * integrate(denom2, 64, s_min, s_max));
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_h_3_naive(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(&BToKstarDilepton<LowRecoil>::differential_h_3, this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max) / (s_max - s_min);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_cp_asymmetry_1(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        std::function<double (const double &)> integrand = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::decay_width, _imp, std::placeholders::_1);

        double gamma = integrate(integrand, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double gamma_bar = integrate(integrand, 64, s_min, s_max);

        return (gamma - gamma_bar) / (gamma + gamma_bar);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_cp_asymmetry_2(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        std::function<double (const double &)> num = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::a_fb_numerator, _imp, std::placeholders::_1);
        std::function<double (const double &)> denom = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::decay_width, _imp, std::placeholders::_1);

        double a_fb = integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double a_fb_bar = integrate(num, 64, s_min, s_max) / integrate(denom, 64, s_min, s_max);

        // Note that in the code A_FB does not flip its sign under CP. Therefore a_fb_bar -> -a_fb_bar here.
        return (a_fb - a_fb_bar) / (a_fb + a_fb_bar);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_cp_asymmetry_3(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        std::function<double (const double &)> u_1 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_1, _imp, std::placeholders::_1);
        std::function<double (const double &)> u_2 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_2, _imp, std::placeholders::_1);
        std::function<double (const double &)> u_5 = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::u_5, _imp, std::placeholders::_1);

        double iu_1 = integrate(u_1, 16, s_min, s_max), iu_2 = integrate(u_2, 16, s_min, s_max), iu_5 = integrate(u_5, 16, s_min, s_max);
        _imp->cp_conjugate = true;
        double iu_1_bar = integrate(u_1, 16, s_min, s_max), iu_2_bar = integrate(u_2, 16, s_min, s_max), iu_5_bar = integrate(u_5, 16, s_min, s_max);

        // Note that in the code U_5 does not flip its sign under CP. Therefore iu_5_bar -> -iu_5_bar here.
        return (iu_5 - iu_5_bar) / std::sqrt((iu_1 + iu_1_bar) * (iu_2 + iu_2_bar));
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_cp_summed_decay_width(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        std::function<double (const double &)> integrand = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::decay_width, _imp, std::placeholders::_1);

        double gamma = integrate(integrand, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double gamma_bar = integrate(integrand, 64, s_min, s_max);

        return (gamma + gamma_bar);
    }

    double
    BToKstarDilepton<LowRecoil>::integrated_unnormalized_cp_asymmetry_1(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);

        std::function<double (const double &)> integrand = std::bind(&Implementation<BToKstarDilepton<LowRecoil>>::decay_width, _imp, std::placeholders::_1);

        double gamma = integrate(integrand, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double gamma_bar = integrate(integrand, 64, s_min, s_max);

        return (gamma - gamma_bar);
    }

    double
    BToKstarDilepton<LowRecoil>::four_differential_decay_width(const double & s, const double & c_theta_l, const double & c_theta_k, const double & phi) const
    {
        // compute J (and d^4 Gamma) for m_l = 0, cf. [BHvD2010], p. 5, Eq. (2.6)
        // and p. 26, Eqs. (A1)-(A11)
        complex<double> a_long_left = _imp->a_long(left_handed, s),
            a_long_right = _imp->a_long(right_handed, s);
        complex<double> a_perp_left = _imp->a_perp(left_handed, s),
            a_perp_right = _imp->a_perp(right_handed, s);
        complex<double> a_par_left = _imp->a_par(left_handed, s),
            a_par_right = _imp->a_par(right_handed, s);
        complex<double> a_timelike = _imp->a_timelike(s);

        double z = 4.0 * power_of<2>(_imp->m_l()) / s, beta2 = 1.0 - z, beta = sqrt(beta2);

        // Cosine squared of the angles
        double c_theta_k_2 = c_theta_k * c_theta_k;
        double c_theta_l_2 = c_theta_l * c_theta_l;
        double c_phi = cos(phi);
        // Sine squared of the angles
        double s_theta_k_2 = 1.0 - c_theta_k_2;
        double s_theta_l_2 = 1.0 - c_theta_l_2;
        // Sine of the angles
        double s_theta_k = sqrt(s_theta_k_2);
        double s_theta_l = sqrt(s_theta_l_2);
        double s_phi = sin(phi);
        // Cosine of twice the angle
        //double c_2_theta_k = 2.0 * c_theta_k - 1.0;
        double c_2_theta_l = 2.0 * c_theta_l - 1.0;
        double c_2_phi = cos(2.0 * phi);
        // Sine of twice the angle
        double s_2_theta_k = 2.0 * s_theta_k * c_theta_k;
        double s_2_theta_l = 2.0 * s_theta_l * c_theta_l;
        double s_2_phi = sin(2.0 * phi);

        double j1s = 3.0 / 4.0 * (
                (2.0 + beta2) / 4.0 * (norm(a_perp_left) + norm(a_perp_right) + norm(a_par_left) * norm(a_par_right))
                + z * real(a_perp_left * conj(a_perp_right) + a_par_left * conj(a_par_right)));
        double j1c = 3.0 / 4.0 * (
                norm(a_long_left) + norm(a_long_right)
                + z * (norm(a_timelike) + 2.0 * real(a_long_left * conj(a_long_right))));
        double j2s = 3 * beta2 / 16.0 * (
                norm(a_perp_left) + norm(a_perp_right) + norm(a_par_left) + norm(a_par_right));
        double j2c = -3.0 * beta2 / 4.0 * (
                norm(a_long_left) + norm(a_long_right));
        double j3 = 3.0 / 8.0 * beta2 * (
                norm(a_perp_left) + norm(a_perp_right) - norm(a_par_left) - norm(a_par_right));
        double j4 = 3.0 / (4.0 * sqrt(2.0)) * beta2 * real(
                a_long_left * conj(a_par_left) + a_long_right * conj(a_par_right));
        double j5 = 3.0 * sqrt(2.0) / 4.0 * beta * real(
                a_long_left * conj(a_perp_left) - a_long_right * conj(a_perp_right));
        double j6s = 3.0 / 2.0 * beta * real(
                a_par_left * conj(a_perp_left) - a_par_right * conj(a_perp_right));
        double j6c = 0.0;
        double j7 = 3.0 * sqrt(2.0) / 4.0 * beta * imag(
                a_long_left * conj(a_par_left) - a_long_right * conj(a_par_right));
        double j8 = 3.0 / 4.0 / sqrt(2.0) * beta2 * imag(
                a_long_left * conj(a_perp_left) + a_long_right * conj(a_perp_right));
        double j9 = 3.0 / 4.0 * beta2 * imag(
                conj(a_par_left) * a_perp_left + conj(a_par_right) * a_perp_right);

        return 3.0 / 8.0 / M_PI * (
                    j1s + (j1c - j1s) * c_theta_k_2
                +  (j2s + (j2c - j2s) * c_theta_k_2) * c_2_theta_l
                +  j3 * s_theta_k_2 * s_theta_l_2 * c_2_phi
                +  j4 * s_2_theta_k * s_2_theta_l * c_phi
                +  j5 * s_2_theta_k * s_theta_l * c_phi
                +  (j6s * s_theta_k_2 + j6c * c_theta_k_2) * c_theta_l
                +  j7 * s_2_theta_k * s_theta_l * s_phi
                +  j8 * s_2_theta_k * s_2_theta_l * s_phi
                +  j9 * s_theta_k_2 * s_theta_l_2 * s_2_phi
                );
    }

    /*
     * Decay: B -> K l lbar at Low Recoil
     */
    template <>
    struct Implementation<BToKDilepton<LowRecoil>>
    {
        Parameters parameters;

        std::shared_ptr<Model> model;

        UsedParameter hbar;

        UsedParameter m_b_MSbar;

        UsedParameter m_B;

        UsedParameter m_K;

        UsedParameter m_l;

        UsedParameter mu;

        UsedParameter alpha_e;

        UsedParameter g_fermi;

        UsedParameter lambda_pseudo;

        UsedParameter sl_phase_pseudo;

        // Mean life time
        UsedParameter tau;

        bool cp_conjugate;

        bool ccbar_resonance;

        std::shared_ptr<FormFactors<PToP>> form_factors;

        Implementation(const Parameters & p, const Options & o, ParameterUser & u) :
            parameters(p),
            model(Model::make(o.get("model", "SM"), p, o)),
            hbar(p["hbar"], u),
            m_b_MSbar(p["mass::b(MSbar)"], u),
            m_B(p["mass::B_" + o.get("q", "d")], u),
            m_K(p["mass::K0"], u),
            m_l(p["mass::" + o.get("l", "mu")], u),
            mu(p["mu"], u),
            alpha_e(p["QED::alpha_e(m_b)"], u),
            g_fermi(p["G_Fermi"], u),
            lambda_pseudo(p["B->Pll::Lambda_pseudo@LowRecoil"], u),
            sl_phase_pseudo(p["B->Pll::sl_phase_pseudo@LowRecoil"], u),
            tau(p["life_time::B_" + o.get("q", "d")], u),
            cp_conjugate(destringify<bool>(o.get("cp-conjugate", "false"))),
            ccbar_resonance(destringify<bool>(o.get("ccbar-resonance", "false")))
        {
            form_factors = FormFactorFactory<PToP>::create("B->K@" + o.get("form-factors", "KMPW2010"), p);

            if (! form_factors.get())
                throw InternalError("Form factors not found!");

            std::string spectator_quark = o.get("q", "d");
            if ((spectator_quark != "d") && (spectator_quark != "u"))
                throw InternalError("Unsupported spectator quark");

            u.uses(*form_factors);
            u.uses(*model);
        }

        // We use the PS mass except for kappa
        double m_b_PS() const
        {
            // Actually use m_b_PS at mu_PS = 2.0 GeV
            return model->m_b_ps(2.0);
        }

        // cf. [GP2004], Eq. (56)
        complex<double> c7eff(const WilsonCoefficients<BToS> & wc, const double & s) const
        {
            return ShortDistanceLowRecoil::c7eff(s, mu(), model->alpha_s(mu), m_b_PS(), true, wc);
        }

        // cf. [GP2004], Eq. (55), p. 10
        complex<double> c9eff(const WilsonCoefficients<BToS> & wc, const double & s) const
        {
            complex<double> lambda_hat_u = (model->ckm_ub() * conj(model->ckm_us())) / (model->ckm_tb() * conj(model->ckm_ts()));

            if (cp_conjugate)
            {
                lambda_hat_u = conj(lambda_hat_u);
            }

            return ShortDistanceLowRecoil::c9eff(s, mu(), model->alpha_s(mu), m_b_PS(), model->m_c_msbar(mu), true, ccbar_resonance, lambda_hat_u, wc);
        }

        double kappa() const
        {
            // cf. [BHvD2010], Eq. (3.8), p. 8
            // Use m_b_MSbar(m_b_MSbar) instead m_b_MSbar(mu), as we want kappa up to NLO only.
            return (1.0 - 2.0 * model->alpha_s(mu) / (3.0 * M_PI) * std::log(mu / m_b_MSbar));
        }

        // this is rho_1^+
        double rho_1(const WilsonCoefficients<BToS> & wc, const double & s) const
        {
            double alpha_s = model->alpha_s(mu());

            return std::norm(kappa() * (2.0 * (m_b_MSbar + lambda_pseudo()) * m_B() / s) * (c7eff(wc, s) + wc.c7prime())
                    + 0.5 * alpha_s * std::polar(lambda_pseudo(), sl_phase_pseudo()) + (c9eff(wc, s) + wc.c9prime()))
                    + std::norm(wc.c10() + wc.c10prime());
        }

        // speed of the lepton
        double beta(const double & s) const
        {
            return std::sqrt(1.0 - 4.0 * power_of<2>(m_l()) / s);
        }

        double xi_b(const double & s) const
        {
            return 1.0 - (power_of<2>(m_B()) - power_of<2>(m_K())) / s * (1.0 - form_factors->f_0(s) / form_factors->f_p(s));
        }

        double gamma0() const
        {
            const double lambda_t_abs = abs(model->ckm_tb() * conj(model->ckm_ts()));

            return power_of<2>(g_fermi() * lambda_t_abs * alpha_e()) / (512.0 * power_of<5>(M_PI) * power_of<3>(m_B()));
        }

        double a_l(const WilsonCoefficients<BToS> & wc, const double & s) const
        {
            const double lam = lambda(power_of<2>(m_B()), power_of<2>(m_K()), s);

            return gamma0() * std::sqrt(lam) * beta(s) * power_of<2>(form_factors->f_p(s)) *
                    (0.25 * lam * rho_1(wc, s) + power_of<2>(m_l()) * std::norm(wc.c10() + wc.c10prime()) * (power_of<2>(xi_b(s)) * s +
                    2.0 * (power_of<2>(m_B()) - power_of<2>(m_K()) - s) * xi_b(s) + 4.0 * power_of<2> (m_K())));
        }

        double c_l(const WilsonCoefficients<BToS> & wc, const double & s) const
        {
            return -0.25 * gamma0() * power_of<2>(form_factors->f_p(s)) * pow(lambda(power_of<2>(m_B()), power_of<2>(m_K()), s), 1.5) *
                    power_of<3>(beta(s)) * rho_1(wc, s);
        }

        double unnormalized_decay_width(const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);

            return 2.0 * (a_l(wc, s) + c_l(wc, s) / 3.0);
        }

        double differential_branching_ratio(const double & s) const
        {
            return unnormalized_decay_width(s) * tau() / hbar();
        }

        double differential_flat_term_numerator(const double & s) const
        {
            WilsonCoefficients<BToS> wc = model->wilson_coefficients_b_to_s(cp_conjugate);

            return 2.0 * (a_l(wc, s) + c_l(wc, s));
        }
    };

    BToKDilepton<LowRecoil>::BToKDilepton(const Parameters & parameters, const Options & options) :
        PrivateImplementationPattern<BToKDilepton<LowRecoil>>(new Implementation<BToKDilepton<LowRecoil>>(parameters, options, *this))
    {
    }

    BToKDilepton<LowRecoil>::~BToKDilepton()
    {
    }

    double
    BToKDilepton<LowRecoil>::real_c9eff(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return real(_imp->c9eff(wc, s));
    }

    double
    BToKDilepton<LowRecoil>::imag_c9eff(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return imag(_imp->c9eff(wc, s));
    }

    double
    BToKDilepton<LowRecoil>::real_c7eff(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return real(_imp->c7eff(wc, s));
    }

    double
    BToKDilepton<LowRecoil>::imag_c7eff(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return imag(_imp->c7eff(wc, s));
    }

    double
    BToKDilepton<LowRecoil>::a_l(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return _imp->a_l(wc, s);
    }

    double
    BToKDilepton<LowRecoil>::c_l(const double & s) const
    {
        WilsonCoefficients<BToS> wc = _imp->model->wilson_coefficients_b_to_s(_imp->cp_conjugate);

        return _imp->c_l(wc, s);
    }

    // Differential Observables
    double
    BToKDilepton<LowRecoil>::differential_branching_ratio(const double & s) const
    {
        return _imp->differential_branching_ratio(s);
    }

    double
    BToKDilepton<LowRecoil>::differential_flat_term(const double & s) const
    {
        return _imp->differential_flat_term_numerator(s) / _imp->unnormalized_decay_width(s);
    }

    // Integrated Observables
    double
    BToKDilepton<LowRecoil>::integrated_branching_ratio(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(std::mem_fn(&BToKDilepton<LowRecoil>::differential_branching_ratio),
                this, std::placeholders::_1);

        return integrate(integrand, 64, s_min, s_max);
    }

    double
    BToKDilepton<LowRecoil>::integrated_branching_ratio_cp_averaged(const double & s_min, const double & s_max) const
    {
        Save<bool> save(_imp->cp_conjugate, false);
        std::function<double (const double &)> integrand = std::bind(&BToKDilepton<LowRecoil>::differential_branching_ratio,
                this, std::placeholders::_1);

        double br = integrate(integrand, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double br_bar = integrate(integrand, 64, s_min, s_max);

        return (br + br_bar) / 2.0;
    }

    double
    BToKDilepton<LowRecoil>::integrated_flat_term(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> num = std::bind(std::mem_fn(&Implementation<BToKDilepton<LowRecoil>>::differential_flat_term_numerator),
                _imp, std::placeholders::_1);
        std::function<double (const double &)> denom = std::bind(std::mem_fn(&Implementation<BToKDilepton<LowRecoil>>::unnormalized_decay_width),
                _imp, std::placeholders::_1);

        double num_integrated = integrate(num, 64, s_min, s_max);
        double denom_integrated = integrate(denom, 64, s_min, s_max);

        return num_integrated / denom_integrated;
    }

    double
    BToKDilepton<LowRecoil>::integrated_ratio_muons_electrons(const double & s_min, const double & s_max) const
    {
        std::function<double (const double &)> integrand = std::bind(std::mem_fn(&BToKDilepton<LowRecoil>::differential_branching_ratio),
                this, std::placeholders::_1);

        double br_electrons;
        {
            Save<Parameter, double> save_m_l(_imp->m_l, _imp->parameters["mass::e"]());
            br_electrons = integrate(integrand, 64, s_min, s_max);
        }

        double br_muons;
        {
            Save<Parameter, double> save_m_l(_imp->m_l, _imp->parameters["mass::mu"]());
            br_muons = integrate(integrand, 64, s_min, s_max);
        }

        // cf. [BHP2007], Eq. (4.10), p. 6
        return br_muons / br_electrons;
    }

    double
    BToKDilepton<LowRecoil>::integrated_cp_asymmetry_1(const double & s_min, const double & s_max) const
    {
        Save<bool> cp_conjugate(_imp->cp_conjugate, false);

        std::function<double (const double &)> integrand = std::bind(std::mem_fn(&Implementation<BToKDilepton<LowRecoil>>::unnormalized_decay_width),
                _imp, std::placeholders::_1);

        double gamma = integrate(integrand, 64, s_min, s_max);
        _imp->cp_conjugate = true;
        double gamma_bar = integrate(integrand, 64, s_min, s_max);

        return (gamma - gamma_bar) / (gamma + gamma_bar);
    }
}