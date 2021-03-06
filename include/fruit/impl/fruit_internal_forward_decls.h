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

#ifndef FRUIT_FRUIT_INTERNAL_FORWARD_DECLS_H
#define FRUIT_FRUIT_INTERNAL_FORWARD_DECLS_H

namespace fruit {

namespace impl {

class ComponentStorage;
class NormalizedComponentStorage;
class InjectorStorage;

template <typename T>
struct NoBindingFoundError;

template <typename... Ts>
struct CheckNoRepeatedTypes;

template <bool has_no_self_loop, typename T, typename... Requirements>
struct CheckHasNoSelfLoop;

template <bool has_no_self_loop, typename T, typename Requirements>
struct CheckHasNoSelfLoopHelper;

template <typename T, typename C>
struct CheckClassType;

template <bool b, typename C>
struct CheckTypeAlreadyBound;

template <typename RequiredSignature, typename SignatureInInjectTypedef>
struct CheckSameSignatureInInjectionTypedef;

template <typename DuplicatedTypes>
struct DuplicatedTypesInComponentError;

template <typename... Requirements>
struct CheckNoRequirementsInInjector;

template <typename Rs>
struct CheckNoRequirementsInInjectorHelper;

template <typename C, typename CandidateSignature>
struct InjectTypedefNotASignature;

template <typename C, typename SignatureReturnType>
struct InjectTypedefForWrongClass;

template <typename CandidateSignature>
struct ParameterIsNotASignature;

template <typename Signature>
struct ConstructorDoesNotExist; // Not used.

template <typename I, typename C>
struct NotABaseClassOf;

template <typename Signature, typename ProviderType>
struct FunctorUsedAsProvider;

template <typename... ComponentRequirements>
struct ComponentWithRequirementsInInjectorError;

template <typename ComponentRequirements>
struct ComponentWithRequirementsInInjectorErrorHelper;

template <typename... UnsatisfiedRequirements>
struct UnsatisfiedRequirementsInNormalizedComponent;

template <typename UnsatisfiedRequirements>
struct UnsatisfiedRequirementsInNormalizedComponentHelper;

template <typename... TypesNotProvided>
struct TypesInInjectorNotProvided;

template <typename TypesNotProvided>
struct TypesInInjectorNotProvidedHelper;

template <typename T, bool is_provided>
struct TypeNotProvidedError;

template <typename C, typename InjectSignature>
struct NoConstructorMatchingInjectSignature;

template <typename ExpectedSignature, typename FunctorSignature>
struct FunctorSignatureDoesNotMatch;

template <bool returns_pointer, typename Signature>
struct FactoryReturningPointer;

} // namespace impl

} // namespace fruit

#endif // FRUIT_FRUIT_INTERNAL_FORWARD_DECLS_H
