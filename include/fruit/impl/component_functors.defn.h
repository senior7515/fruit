/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FRUIT_COMPONENT_FUNCTORS_DEFN_H
#define FRUIT_COMPONENT_FUNCTORS_DEFN_H

#include "../component.h"

#include "injection_errors.h"
#include "storage/component_storage.h"

#include <memory>

namespace fruit {
namespace impl {

template <typename Op, typename Comp>
using ApplyFunctor = typename Op::template apply<Comp>;

struct Identity {
  template <typename Comp>
  struct apply {
    using Result = Comp;
    void operator()(ComponentStorage& storage) {
      (void)storage;
    }
  };
};

template <typename... Functors>
struct ComposeFunctorsHelper;

template <typename F, typename... Functors>
struct ComposeFunctorsHelper<F, Functors...> {
  template <typename Comp>
  struct apply {
    using Op = ApplyFunctor<F, Comp>;
    using Comp1 = typename Op::Result;
    using Ops = ApplyFunctor<ComposeFunctorsHelper<Functors...>, Comp1>;
    using Comp2 = typename Ops::Result;
    using Result = Comp2;
    void operator()(ComponentStorage& storage) {
      Op()(storage);
      Ops()(storage);
    }
  };
};

template <>
struct ComposeFunctorsHelper<> : Identity {
};

template <typename... Ops>
using ComposeFunctors = ComposeFunctorsHelper<Ops...>;

template <typename TargetRequirements, typename T>
struct EnsureProvidedType;

template <typename TargetRequirements, typename L>
struct EnsureProvidedTypes;

// Doesn't actually bind in ComponentStorage. The binding is added later (if needed) using ProcessInterfaceBinding.
template <typename I, typename C>
struct AddDeferredInterfaceBinding {
  template <typename Comp>
  struct apply {
    using Result = meta::ConsComp<typename Comp::Rs,
                                  typename Comp::Ps,
                                  typename Comp::Deps,
                                  meta::Apply<meta::AddToSet,
                                        meta::ConsInterfaceBinding<I, C>,
                                        typename Comp::InterfaceBindings>,
                                  typename Comp::DeferredBindingFunctors>;
    void operator()(ComponentStorage& storage) {
      (void)storage;
    };
  };
};

template <typename I, typename C>
struct ProcessInterfaceBinding {
  template <typename Comp>
  struct apply {
    FruitDelegateCheck(NotABaseClassOf<I, C>);
    using Result = meta::Apply<meta::AddProvidedType, Comp, I, meta::Vector<C>>;
    void operator()(ComponentStorage& storage) {
      storage.addBinding(InjectorStorage::createBindingDataForBind<I, C>());
    };
  };
};

template <typename I, typename C>
struct AddInterfaceMultibinding {
  template <typename Comp>
  struct apply {
    FruitDelegateCheck(NotABaseClassOf<I, C>);
    using Result = meta::Apply<meta::AddRequirements, Comp, meta::Vector<C>>;
    void operator()(ComponentStorage& storage) {
      storage.addMultibinding(InjectorStorage::createMultibindingDataForBinding<I, C>());
    };
  };
};

template <typename Lambda, typename OptionalI>
struct ProcessRegisterProviderHelper {
  inline void operator()(ComponentStorage& component) {
    component.addBinding(InjectorStorage::createBindingDataForProvider<Lambda>());
    component.addCompressedBinding(InjectorStorage::createBindingDataForCompressedProvider<Lambda, OptionalI>());
  }
};

template <typename Lambda>
struct ProcessRegisterProviderHelper<Lambda, meta::None> {
  inline void operator()(ComponentStorage& component) {
    component.addBinding(InjectorStorage::createBindingDataForProvider<Lambda>());
  }
};

// T can't be any injectable type, it must match the return type of the provider in one of
// the registerProvider() overloads in ComponentStorage.
template <typename Lambda>
struct ProcessRegisterProvider {
  template <typename Comp>
  struct apply {
    using Signature = meta::Apply<meta::FunctionSignature, Lambda>;
    using C = meta::Apply<meta::GetClassForType, meta::Apply<meta::SignatureType, Signature>>;
    using Result = meta::Apply<meta::AddProvidedType,
                              Comp,
                              C,
                              meta::Apply<meta::SignatureArgs, Signature>>;
    void operator()(ComponentStorage& storage) {
      ProcessRegisterProviderHelper<Lambda, meta::Apply<meta::GetBindingToInterface, C, typename Comp::InterfaceBindings>>()(storage);
    }
  };
};

// The registration is actually deferred until the PartialComponent is converted to a component.
template <typename Lambda>
struct DeferredRegisterProvider {
  template <typename Comp>
  struct apply {
    using Result = meta::Apply<meta::AddDeferredBinding, Comp, ProcessRegisterProvider<Lambda>>;
    void operator()(ComponentStorage& storage) {
      (void) storage;
    }
  };
};

// T can't be any injectable type, it must match the return type of the provider in one of
// the registerMultibindingProvider() overloads in ComponentStorage.
template <typename Lambda>
struct RegisterMultibindingProvider {
  template <typename Comp>
  struct apply {
    using Result = meta::Apply<meta::AddRequirements,
                              Comp,
                              meta::Apply<meta::SignatureArgs, meta::Apply<meta::FunctionSignature, Lambda>>>;
    void operator()(ComponentStorage& storage) {
      storage.addMultibinding(InjectorStorage::createMultibindingDataForProvider<Lambda>());
    }
  };
};

// Non-assisted case.
template <int numAssistedBefore, int numNonAssistedBefore, typename Arg, typename InjectedArgsTuple, typename UserProvidedArgsTuple>
struct GetAssistedArg {
  inline Arg operator()(InjectedArgsTuple& injected_args, UserProvidedArgsTuple&) {
    return std::get<numNonAssistedBefore>(injected_args);
  }
};

// Assisted case.
template <int numAssistedBefore, int numNonAssistedBefore, typename Arg, typename InjectedArgsTuple, typename UserProvidedArgsTuple>
struct GetAssistedArg<numAssistedBefore, numNonAssistedBefore, Assisted<Arg>, InjectedArgsTuple, UserProvidedArgsTuple> {
  inline Arg operator()(InjectedArgsTuple&, UserProvidedArgsTuple& user_provided_args) {
    return std::get<numAssistedBefore>(user_provided_args);
  }
};

template <typename AnnotatedSignature,
          typename Lambda,
          typename InjectedSignature = meta::Apply<meta::InjectedSignatureForAssistedFactory, AnnotatedSignature>,
          typename RequiredSignature = meta::Apply<meta::RequiredSignatureForAssistedFactory, AnnotatedSignature>,
          typename InjectedArgs = meta::Apply<meta::RemoveAssisted, meta::Apply<meta::SignatureArgs, AnnotatedSignature>>,
          typename IndexSequence = meta::GenerateIntSequence<
              meta::ApplyC<meta::VectorSize,
                  meta::Apply<meta::RequiredArgsForAssistedFactory, AnnotatedSignature>
              >::value>
          >
struct RegisterFactory;

template <typename AnnotatedSignature, typename Lambda, typename C, typename... UserProvidedArgs, typename... AllArgs, typename... InjectedArgs, int... indexes>
struct RegisterFactory<AnnotatedSignature, Lambda, C(UserProvidedArgs...), C(AllArgs...), meta::Vector<InjectedArgs...>, meta::IntVector<indexes...>> {
  template <typename Comp>
  struct apply {
    using T = meta::Apply<meta::SignatureType, AnnotatedSignature>;
    using AnnotatedArgs = meta::Apply<meta::SignatureArgs, AnnotatedSignature>;
    using InjectedSignature = C(UserProvidedArgs...);
    using RequiredSignature = C(AllArgs...);
    FruitDelegateCheck(FunctorSignatureDoesNotMatch<RequiredSignature, meta::Apply<meta::FunctionSignature, Lambda>>);
    FruitDelegateCheck(FactoryReturningPointer<std::is_pointer<T>::value, AnnotatedSignature>);
    using fun_t = std::function<InjectedSignature>;
    using Result = meta::Apply<meta::AddProvidedType,
                              Comp,
                              fun_t,
                              meta::Apply<meta::SignatureArgs, AnnotatedSignature>>;
    void operator()(ComponentStorage& storage) {
      auto function_provider = [](InjectedArgs... args) {
        // TODO: Using auto and make_tuple here results in a GCC segfault with GCC 4.8.1.
        // Check this on later versions and consider filing a bug.
        std::tuple<InjectedArgs...> injected_args(args...);
        auto object_provider = [injected_args](UserProvidedArgs... params) mutable {
          auto user_provided_args = std::tie(params...);
          // These are unused if they are 0-arg tuples. Silence the unused-variable warnings anyway.
          (void) injected_args;
          (void) user_provided_args;
          
          return LambdaInvoker::invoke<Lambda, AllArgs...>(GetAssistedArg<
                                                              meta::NumAssistedBefore<indexes, AnnotatedArgs>::value,
                                                              indexes - meta::NumAssistedBefore<indexes, AnnotatedArgs>::value,
                                                              meta::GetNthType<indexes, AnnotatedArgs>,
                                                              decltype(injected_args),
                                                              decltype(user_provided_args)
                                                          >()(injected_args, user_provided_args)
                                                          ...);
        };
        return fun_t(object_provider);
      };  
      storage.addBinding(InjectorStorage::createBindingDataForProvider<decltype(function_provider)>());
    }
  };
};

// TODO: Check if this definition is still needed or if a forward-decl would suffice.
template <typename Signature>
struct ProcessRegisterConstructor {
  template <typename Comp>
  struct apply {
    // Something is wrong. We provide a Result and an operator() here to avoid backtracking with SFINAE, and to allow the function
    // that instantiated this class to return the appropriate error.
    using Result = int;
    // This method is not implemented.
    int operator()(ComponentStorage& component);
  };
};

template <typename Signature, typename OptionalI>
struct ProcessRegisterConstructorHelper {
  inline void operator()(ComponentStorage& component) {
    component.addBinding(InjectorStorage::createBindingDataForConstructor<Signature>());
    component.addCompressedBinding(InjectorStorage::createBindingDataForCompressedConstructor<Signature, OptionalI>());
  }
};

template <typename Signature>
struct ProcessRegisterConstructorHelper<Signature, meta::None> {
  inline void operator()(ComponentStorage& component) {
    component.addBinding(InjectorStorage::createBindingDataForConstructor<Signature>());
  }
};

template <typename T, typename... Args>
struct ProcessRegisterConstructor<T(Args...)> {
  template <typename Comp>
  struct apply {
    using C = meta::Apply<meta::GetClassForType, T>;
    using Result = meta::Apply<meta::AddProvidedType, Comp, C, meta::Vector<Args...>>;
    void operator()(ComponentStorage& storage) {
      ProcessRegisterConstructorHelper<T(Args...), meta::Apply<meta::GetBindingToInterface, C, typename Comp::InterfaceBindings>>()(storage);
    }
  };
};

template <typename Signature>
struct DeferredRegisterConstructor {
  template <typename Comp>
  struct apply {
    using Result = meta::Apply<meta::AddDeferredBinding, Comp, ProcessRegisterConstructor<Signature>>;
    void operator()(ComponentStorage& storage) {
      (void) storage;
    }
  };
};

template <typename C>
struct RegisterInstance {
  template <typename Comp>
  struct apply {
    using Result = meta::Apply<meta::AddProvidedType, Comp, C, meta::Vector<>>;
    void operator()(ComponentStorage& storage, C& instance) {
      storage.addBinding(InjectorStorage::createBindingDataForBindInstance<C>(instance));
    };
  };
};

template <typename C>
struct AddInstanceMultibinding {
  template <typename Comp>
  struct apply {
    using Result = Comp;
    void operator()(ComponentStorage& storage, C& instance) {
      storage.addMultibinding(InjectorStorage::createMultibindingDataForInstance<C>(instance));
    };
  };
};

template <typename Comp, typename AnnotatedSignature, typename RequiredSignature>
struct RegisterConstructorAsValueFactoryHelper;

template <typename Comp, typename AnnotatedSignature, typename T, typename... Args>
struct RegisterConstructorAsValueFactoryHelper<Comp, AnnotatedSignature, T(Args...)> {
  void operator()(ComponentStorage& storage) {
    auto provider = [](Args... args) {
      return T(std::forward<Args>(args)...);
    };
    using RealF1 = RegisterFactory<AnnotatedSignature, decltype(provider)>;
    using RealOp = ApplyFunctor<RealF1, Comp>;
    RealOp()(storage);
  }
};

template <typename AnnotatedSignature>
struct RegisterConstructorAsValueFactory {
  template <typename Comp>
  struct apply {
    using RequiredSignature = meta::Apply<meta::ConstructSignature,
                                          meta::Apply<meta::SignatureType, AnnotatedSignature>,
                                          meta::Apply<meta::RequiredArgsForAssistedFactory, AnnotatedSignature>>;
    using F1 = RegisterFactory<AnnotatedSignature, RequiredSignature>;
    using Op = ApplyFunctor<F1, Comp>;
    using Result = typename Op::Result;
    void operator()(ComponentStorage& storage) {
      RegisterConstructorAsValueFactoryHelper<Comp, AnnotatedSignature, RequiredSignature>()(storage);
    };
  };
};

template <typename AnnotatedSignature,
          typename RequiredSignature = meta::Apply<meta::ConstructSignature,
                                                   std::unique_ptr<meta::Apply<meta::SignatureType, AnnotatedSignature>>,
                                                   meta::Apply<meta::RequiredArgsForAssistedFactory, AnnotatedSignature>>>
struct RegisterConstructorAsPointerFactory;

template <typename AnnotatedSignature, typename T, typename... Args>
struct RegisterConstructorAsPointerFactory<AnnotatedSignature, std::unique_ptr<T>(Args...)> {
  template <typename Comp>
  struct apply {
    using RequiredSignature = std::unique_ptr<T>(Args...);
    using F1 = RegisterFactory<AnnotatedSignature, RequiredSignature>;
    using Op = ApplyFunctor<F1, Comp>;
    using Result = typename Op::Result;
    void operator()(ComponentStorage& storage) {
      auto provider = [](Args... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
      };
      using RealF1 = RegisterFactory<AnnotatedSignature, decltype(provider)>;
      using RealOp = ApplyFunctor<RealF1, Comp>;
      static_assert(std::is_same<typename Op::Result,
                                 typename RealOp::Result>::value,
                    "Fruit bug, F1 and RealF1 out of sync.");
      RealOp()(storage);
    };
  };
};

template <typename OtherComp>
struct InstallComponent {
  template <typename Comp>
  struct apply {
    FruitDelegateCheck(DuplicatedTypesInComponentError<meta::Apply<meta::SetIntersection, typename OtherComp::Ps, typename Comp::Ps>>);
    using new_Ps = meta::Apply<meta::ConcatVectors, typename OtherComp::Ps, typename Comp::Ps>;
    using new_Rs = meta::Apply<meta::SetDifference,
                              meta::Apply<meta::SetUnion,
                                          typename OtherComp::Rs,
                                          typename Comp::Rs>,
                                          new_Ps>;
    using new_Deps = 
        meta::Apply<meta::AddProofTreeVectorToForest, typename OtherComp::Deps, typename Comp::Deps, typename Comp::Ps>;
    using new_InterfaceBindings = 
        meta::Apply<meta::SetUnion, typename OtherComp::InterfaceBindings, typename Comp::InterfaceBindings>;
    using new_DeferredBindingFunctors = 
        meta::Apply<meta::ConcatVectors, typename OtherComp::DeferredBindingFunctors, typename Comp::DeferredBindingFunctors>;
    using Result = meta::ConsComp<new_Rs, new_Ps, new_Deps, new_InterfaceBindings, new_DeferredBindingFunctors>;
    void operator()(ComponentStorage& storage, ComponentStorage&& other_storage) {
      storage.install(std::move(other_storage));
    }
  };
};

// Used to limit the amount of metaprogramming in component.h, that might confuse users.
template <typename... OtherCompParams>
struct InstallComponentHelper : public InstallComponent<meta::Apply<meta::ConstructComponentImpl, OtherCompParams...>> {
};

template <typename DestComp>
struct ConvertComponent {
  template <typename SourceComp>
  struct apply {
    // We need to register:
    // * All the types provided by the new component
    // * All the types required by the old component
    // except:
    // * The ones already provided by the old component.
    // * The ones required by the new one.
    using ToRegister = meta::Apply<meta::SetDifference,
                                   meta::Apply<meta::SetUnion, typename DestComp::Ps, typename SourceComp::Rs>,
                                   meta::Apply<meta::SetUnion, typename DestComp::Rs, typename SourceComp::Ps>>;
    using F1 = EnsureProvidedTypes<typename DestComp::Rs, ToRegister>;
    using Op = ApplyFunctor<F1, SourceComp>;
    using Result = typename Op::Result;
    
    // Not needed, just double-checking.
    // Uses FruitStaticAssert instead of FruitDelegateCheck so that it's checked only in debug mode.
    FruitStaticAssert(true || sizeof(CheckComponentEntails<Result, DestComp>), "");
    
    void operator()(ComponentStorage& storage) {
      Op()(storage);
    }
  };
};

struct ProcessDeferredBindings {
  template <typename Comp>
  struct apply;
  
  template <typename RsParam, typename PsParam, typename DepsParam, typename InterfaceBindingsParam, 
            typename... DeferredBindingFunctors>
  struct apply<meta::ConsComp<RsParam, PsParam, DepsParam, InterfaceBindingsParam, meta::Vector<DeferredBindingFunctors...>>> {
    // Comp1 is the same as Comp, but without the DeferredBindingFunctors.
    using Comp1 = meta::ConsComp<RsParam, PsParam, DepsParam, InterfaceBindingsParam, meta::Vector<>>;
    using Op = ApplyFunctor<ComposeFunctors<DeferredBindingFunctors...>, Comp1>;
    using Result = typename Op::Result;
    void operator()(ComponentStorage& storage) {
      Op()(storage);
    }
  };
};

// The types in TargetRequirements will not be auto-registered.
template <typename TargetRequirements, typename C>
struct AutoRegister;

template <typename TargetRequirements, bool has_inject_annotation, typename C>
struct AutoRegisterHelper;

// C has an Inject typedef, use it.
template <typename TargetRequirements, typename C>
struct AutoRegisterHelper<TargetRequirements, true, C> {
  using Inject = meta::Apply<meta::GetInjectAnnotation, C>;
  using F = ComposeFunctors<
                ProcessRegisterConstructor<Inject>,
                EnsureProvidedTypes<TargetRequirements,
                                    meta::Apply<meta::ExpandProvidersInParams, meta::Apply<meta::SignatureArgs, Inject>>>
            >;
  template <typename Comp>
  using apply = ApplyFunctor<F, Comp>;
};

template <typename TargetRequirements, typename C>
struct AutoRegisterHelper<TargetRequirements, false, C> {
  template <typename Comp>
  struct apply {
    FruitDelegateCheck(NoBindingFoundError<C>);
  };
};

template <typename TargetRequirements, bool has_interface_binding, bool has_inject_annotation, typename C, typename... Args>
struct AutoRegisterFactoryHelper;

// I has an interface binding, use it and look for a factory that returns the type that I is bound to.
template <typename TargetRequirements, bool unused, typename I, typename... Argz>
struct AutoRegisterFactoryHelper<TargetRequirements, true, unused, std::unique_ptr<I>, Argz...> {
  template <typename Comp>
  struct apply {
    using C = meta::Apply<meta::GetInterfaceBinding, I, typename Comp::InterfaceBindings>;
    using original_function_t = std::function<std::unique_ptr<C>(Argz...)>;
    using function_t = std::function<std::unique_ptr<I>(Argz...)>;
    
    using F1 = EnsureProvidedType<TargetRequirements, original_function_t>;
    using F2 = ProcessRegisterProvider<function_t*(original_function_t*)>;
    using Op = ApplyFunctor<ComposeFunctors<F1, F2>, Comp>;
    using Result = typename Op::Result;
    void operator()(ComponentStorage& storage) {
      auto provider = [](original_function_t* fun) {
        return new function_t([=](Argz... args) {
          C* c = (*fun)(args...).release();
          I* i = static_cast<I*>(c);
          return std::unique_ptr<I>(i);
        });
      };
      using RealF2 = ProcessRegisterProvider<decltype(provider)>;
      using RealOp = ApplyFunctor<ComposeFunctors<F1, RealF2>, Comp>;
      static_assert(std::is_same<typename Op::Result,
                                 typename RealOp::Result>::value,
                    "Fruit bug, F2 and RealF2 out of sync.");
      RealOp()(storage);
    }
  };
};

// C doesn't have an interface binding as interface, nor an INJECT annotation.
// Bind std::function<unique_ptr<C>(Args...)> to std::function<C(Args...)>.
template <typename TargetRequirements, typename C, typename... Argz>
struct AutoRegisterFactoryHelper<TargetRequirements, false, false, std::unique_ptr<C>, Argz...> {
  using original_function_t = std::function<C(Argz...)>;
  using function_t = std::function<std::unique_ptr<C>(Argz...)>;
  
  using F1 = EnsureProvidedType<TargetRequirements, original_function_t>;
  using F2 = ProcessRegisterProvider<function_t*(original_function_t*)>;
  
  template <typename Comp>
  struct apply {
    using Op = ApplyFunctor<ComposeFunctors<F1, F2>, Comp>;
    using Result = typename Op::Result;
    void operator()(ComponentStorage& storage) {
      auto provider = [](original_function_t* fun) {
        return new function_t([=](Argz... args) {
          C* c = new C((*fun)(args...));
          return std::unique_ptr<C>(c);
        });
      };
      using RealF2 = ProcessRegisterProvider<decltype(provider)>;
      using RealOp = ApplyFunctor<ComposeFunctors<F1, RealF2>, Comp>;
      static_assert(std::is_same<typename Op::Result, typename RealOp::Result>::value,
                    "Fruit bug, F2 and RealF2 out of sync.");
      RealOp()(storage);
    }
  };
};

// C has an Inject typedef, use it. unique_ptr case.
// TODO: Doesn't work after renaming Argz->Args, consider minimizing the test case and filing a bug.
template <typename TargetRequirements, typename C, typename... Argz>
struct AutoRegisterFactoryHelper<TargetRequirements, false, true, std::unique_ptr<C>, Argz...> {
  using AnnotatedSignature = meta::Apply<meta::GetInjectAnnotation, C>;
  using AnnotatedSignatureArgs = meta::Apply<meta::SignatureArgs, AnnotatedSignature>;
  FruitDelegateCheck(CheckSameSignatureInInjectionTypedef<
      meta::Apply<meta::ConstructSignature, C, meta::Vector<Argz...>>,
      meta::Apply<meta::ConstructSignature, C, meta::Apply<meta::RemoveNonAssisted, AnnotatedSignatureArgs>>>);
  using NonAssistedArgs = meta::Apply<meta::RemoveAssisted, AnnotatedSignatureArgs>;
  
  using F1 = RegisterConstructorAsPointerFactory<AnnotatedSignature>;
  using F2 = EnsureProvidedTypes<TargetRequirements,
                                 meta::Apply<meta::ExpandProvidersInParams, NonAssistedArgs>>;
  
  template <typename Comp>
  using apply = ApplyFunctor<ComposeFunctors<F1, F2>, Comp>;
};

// C has an Inject typedef, use it. Value (not unique_ptr) case.
// TODO: Doesn't work after renaming Argz->Args, consider minimizing the test case and filing a bug.
template <typename TargetRequirements, typename C, typename... Argz>
struct AutoRegisterFactoryHelper<TargetRequirements, false, true, C, Argz...> {
  using AnnotatedSignature = meta::Apply<meta::GetInjectAnnotation, C>;
  using AnnotatedSignatureArgs = meta::Apply<meta::SignatureArgs, AnnotatedSignature>;
  FruitDelegateCheck(CheckSameSignatureInInjectionTypedef<
      meta::Apply<meta::ConstructSignature, C, meta::Vector<Argz...>>,
      meta::Apply<meta::ConstructSignature, C, meta::Apply<meta::RemoveNonAssisted, AnnotatedSignatureArgs>>>);
  using NonAssistedArgs = meta::Apply<meta::RemoveAssisted, AnnotatedSignatureArgs>;
  
  using F1 = RegisterConstructorAsValueFactory<AnnotatedSignature>;
  using F2 = EnsureProvidedTypes<TargetRequirements, meta::Apply<meta::ExpandProvidersInParams, NonAssistedArgs>>;
  
  template <typename Comp>
  using apply = ApplyFunctor<ComposeFunctors<F1, F2>, Comp>;
};

template <typename TargetRequirements, typename C, typename... Args>
struct AutoRegisterFactoryHelper<TargetRequirements, false, false, C, Args...> {
  template <typename Comp>
  struct apply {
    FruitDelegateCheck(NoBindingFoundError<std::function<C(Args...)>>);
  };
};

// Tries to registers C by looking for a typedef called Inject inside C.
template <typename TargetRequirements, typename C>
struct AutoRegister : public AutoRegisterHelper<
      TargetRequirements,
      meta::ApplyC<meta::HasInjectAnnotation, C>::value,
      C
>{};

template <typename TargetRequirements, typename C, typename... Args>
struct AutoRegister<TargetRequirements, std::function<C(Args...)>> {
  template <typename Comp>
  using apply = ApplyFunctor<AutoRegisterFactoryHelper<
      TargetRequirements,
      meta::ApplyC<meta::HasInterfaceBinding, C, typename Comp::InterfaceBindings>::value,
      meta::ApplyC<meta::HasInjectAnnotation, C>::value,
      C,
      Args...>,
      
      Comp>;
};    

template <typename TargetRequirements, typename C, typename... Args>
struct AutoRegister<TargetRequirements, std::function<std::unique_ptr<C>(Args...)>> {
  template <typename Comp>
  using apply = ApplyFunctor<AutoRegisterFactoryHelper<
      TargetRequirements,
      meta::ApplyC<meta::HasInterfaceBinding, C, typename Comp::InterfaceBindings>::value,
      false,
      std::unique_ptr<C>,
      Args...>,
      
      Comp>;
};

template <typename TargetRequirements, bool is_already_provided_or_in_target_requirements, bool has_interface_binding, typename C>
struct EnsureProvidedTypeHelper;

// Already provided or in target requirements, ok.
template <typename TargetRequirements, bool unused, typename C>
struct EnsureProvidedTypeHelper<TargetRequirements, true, unused, C> : public Identity {};

// Has an interface binding.
template <typename TargetRequirements, typename I>
struct EnsureProvidedTypeHelper<TargetRequirements, false, true, I> {
  template <typename Comp>
  struct apply {
    using C = meta::Apply<meta::GetInterfaceBinding, I, typename Comp::InterfaceBindings>;
    using F1 = ProcessInterfaceBinding<I, C>;
    using F2 = EnsureProvidedType<TargetRequirements, C>;
    using Op = ApplyFunctor<ComposeFunctors<F1, F2>, Comp>;
    using Result = typename Op::Result;
    void operator()(ComponentStorage& storage) {
      Op()(storage);
    }
  };
};

// Not yet provided, nor in target requirements, nor in InterfaceBindings. Try auto-registering.
template <typename TargetRequirements, typename C>
struct EnsureProvidedTypeHelper<TargetRequirements, false, false, C> : public AutoRegister<TargetRequirements, C> {};

template <typename TargetRequirements, typename T>
struct EnsureProvidedType {
  using C = meta::Apply<meta::GetClassForType, T>;
  template <typename Comp>
  using apply = ApplyFunctor<EnsureProvidedTypeHelper<TargetRequirements,
                                                      meta::ApplyC<meta::IsInVector, C, typename Comp::Ps>::value
                                                      || meta::ApplyC<meta::IsInVector, C, TargetRequirements>::value,
                                                      meta::ApplyC<meta::HasInterfaceBinding, C, typename Comp::InterfaceBindings>::value,
                                                      C>,
                             Comp>;
};

// General case, empty list.
template <typename TargetRequirements, typename L>
struct EnsureProvidedTypes : public Identity {
  FruitStaticAssert(meta::ApplyC<meta::IsEmptyVector, L>::value, "Implementation error");
};

template <typename TargetRequirements, typename... Ts>
struct EnsureProvidedTypes<TargetRequirements, meta::Vector<meta::None, Ts...>> 
  : public EnsureProvidedTypes<TargetRequirements, meta::Vector<Ts...>> {
};


template <typename TargetRequirements, typename T, typename... Ts>
struct EnsureProvidedTypes<TargetRequirements, meta::Vector<T, Ts...>> {
  using F1 = EnsureProvidedType<TargetRequirements, T>;
  using F2 = EnsureProvidedTypes<TargetRequirements, meta::Vector<Ts...>>;
  template <typename Comp>
  using apply = ApplyFunctor<ComposeFunctors<F1, F2>, Comp>;
};

} // namespace impl
} // namespace fruit


#endif // FRUIT_COMPONENT_FUNCTORS_DEFN_H
