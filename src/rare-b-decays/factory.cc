/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <src/rare-b-decays/factory.hh>
#include <src/rare-b-decays/exclusive-b-to-s-dilepton.cc>
//#include <src/rare-b-decays/inclusive-b-to-s-dilepton.cc>

namespace wf
{
    ObservablePtr
    RareBFactory::make(const std::string & name, const Parameters & parameters, const ObservableOptions & options)
    {
        static const std::map<std::string, ObservableFactory *> simple_observables
        {
#ifdef OBSERVABLE1
# undef OBSERVABLE1
#else
# define OBSERVABLE1(name_, decay_, function_, var1_) \
            std::make_pair(name_, new ConcreteObservableFactory<decay_, 1>( \
                        ConcreteObservableData<decay_, 1>(name_, std::tr1::mem_fn(&decay_::function_), var1_)))
# define OBSERVABLE2(name_, decay_, function_, var1_, var2_) \
            std::make_pair(name_, new ConcreteObservableFactory<decay_, 2>( \
                        ConcreteObservableData<decay_, 2>(name_, std::tr1::mem_fn(&decay_::function_), var1_, var2_)))
#endif

            // B -> K^* ll, Large Recoil
            OBSERVABLE1("B->K^*ll::dBR/ds@LargeRecoil",  BToKstarDilepton<LargeRecoil>, differential_branching_ratio,            "s"),
            OBSERVABLE1("B->K^*ll::A_FB(s)@LargeRecoil", BToKstarDilepton<LargeRecoil>, differential_forward_backward_asymmetry, "s"),
            OBSERVABLE1("B->K^*ll::F_L(s)@LargeRecoil",  BToKstarDilepton<LargeRecoil>, differential_longitudinal_polarisation,  "s"),
            OBSERVABLE2("B->K^*ll::A_FB@LargeRecoil",    BToKstarDilepton<LargeRecoil>, integrated_forward_backward_asymmetry,   "s_min", "s_max"),
            OBSERVABLE2("B->K^*ll::BR@LargeRecoil",      BToKstarDilepton<LargeRecoil>, integrated_branching_ratio,              "s_min", "s_max"),

            // B -> K^* ll, Low Recoil
            OBSERVABLE1("B->K^*ll::dBR/ds@LowRecoil",  BToKstarDilepton<LowRecoil>, differential_branching_ratio,            "s"),
            OBSERVABLE1("B->K^*ll::A_FB(s)@LowRecoil", BToKstarDilepton<LowRecoil>, differential_forward_backward_asymmetry, "s"),
            OBSERVABLE1("B->K^*ll::F_L(s)@LowRecoil",  BToKstarDilepton<LowRecoil>, differential_longitudinal_polarisation,  "s"),
            OBSERVABLE2("B->K^*ll::A_FB@LowRecoil",    BToKstarDilepton<LowRecoil>, integrated_forward_backward_asymmetry,   "s_min", "s_max"),
            OBSERVABLE2("B->K^*ll::BR@LowRecoil",      BToKstarDilepton<LowRecoil>, integrated_branching_ratio,              "s_min", "s_max"),
        };

        ObservableOptions myoptions(options);
        if (! myoptions.has("form-factors"))
            myoptions.set("form-factors", "BZ2004");

        auto i(simple_observables.find(name));
        if (simple_observables.end() == i)
            return std::tr1::shared_ptr<Observable>();

        return i->second->make(parameters, myoptions);
    }
}