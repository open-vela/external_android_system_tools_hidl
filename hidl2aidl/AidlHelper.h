/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include "Reference.h"

namespace android {

struct CompoundType;
struct Coordinator;
struct Formatter;
struct FQName;
struct Interface;
struct Method;
struct NamedType;
struct Scope;
struct Type;

struct FieldWithVersion {
    const NamedReference<Type>* field;
    // name of the field appended by the
    std::string fullName;
    std::pair<size_t, size_t> version;
};

struct ProcessedCompoundType {
    // map modified name to field. This modified name is old.new
    std::vector<FieldWithVersion> fields;
    std::set<const NamedType*> subTypes;
};

struct AidlHelper {
    /* FQName helpers */
    // getAidlName returns the type names
    // android.hardware.foo@1.0::IBar.Baz -> IBarBaz
    static std::string getAidlName(const FQName& fqName);

    // getAidlPackage returns the AIDL package
    // android.hardware.foo@1.x -> android.hardware.foo
    // android.hardware.foo@2.x -> android.hardware.foo2
    static std::string getAidlPackage(const FQName& fqName);
    // returns getAidlPackage(fqName) with '.' replaced by '/'
    // android.hardware.foo@1.x -> android/hardware/foo
    static std::string getAidlPackagePath(const FQName& fqName);

    // getAidlFQName = getAidlPackage + "." + getAidlName
    static std::string getAidlFQName(const FQName& fqName);

    static void emitFileHeader(
            Formatter& out, const NamedType& type,
            const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes);
    static void importLocallyReferencedType(const Type& type, std::set<std::string>* imports);
    static Formatter getFileWithHeader(
            const NamedType& namedType, const Coordinator& coordinator,
            const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes);

    /* Methods for Type */
    static std::string getAidlType(const Type& type, const FQName& relativeTo);

    /* Methods for NamedType */
    static void emitAidl(
            const NamedType& namedType, const Coordinator& coordinator,
            const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes);

    /* Methods for Interface */
    static void emitAidl(const Interface& interface, const Coordinator& coordinator,
                         const std::map<const NamedType*, const ProcessedCompoundType>&);
    // Returns all methods that would exist in an AIDL equivalent interface
    static std::vector<const Method*> getUserDefinedMethods(const Interface& interface);

    static void processCompoundType(const CompoundType& compoundType,
                                    ProcessedCompoundType* processedType,
                                    const std::string& fieldNamePrefix);

    static Formatter& notes();
    static void setNotes(Formatter* formatter);

    static Formatter& translatorHeader();
    static Formatter& translatorSource();
    static void setTranslateHeader(Formatter* formatter);
    static void setTranslateSource(Formatter* formatter);

    static void emitH2aTranslation(
            const std::set<const NamedType*>& namedTypesInPackage,
            const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes);

  private:
    // This is the formatter to use for additional conversion output
    static Formatter* notesFormatter;
    static Formatter* translateHeaderFormatter;
    static Formatter* translateSourceFormatter;
};

}  // namespace android
