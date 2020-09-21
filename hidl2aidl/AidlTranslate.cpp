/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <hidl-util/FQName.h>
#include <hidl-util/Formatter.h>
#include <hidl-util/StringHelper.h>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "AidlHelper.h"
#include "NamedType.h"
#include "ScalarType.h"
#include "Scope.h"

namespace android {

Formatter& AidlHelper::translatorHeader() {
    CHECK(translateHeaderFormatter != nullptr);
    return *translateHeaderFormatter;
}

Formatter& AidlHelper::translatorSource() {
    CHECK(translateSourceFormatter != nullptr);
    return *translateSourceFormatter;
}

void AidlHelper::setTranslateSource(Formatter* formatter) {
    CHECK(formatter != nullptr);
    translateSourceFormatter = formatter;
}

void AidlHelper::setTranslateHeader(Formatter* formatter) {
    CHECK(formatter != nullptr);
    translateHeaderFormatter = formatter;
}

static const std::string namedTypeTranslation(const std::set<const NamedType*>& namedTypes,
                                              const NamedType* type,
                                              const FieldWithVersion& field) {
    if (namedTypes.find(type) == namedTypes.end()) {
        AidlHelper::notes() << "An unknown named type was found in translation: "
                            << type->fqName().string() + "\n";
        return "#error FIXME Unknown type: " + type->fqName().string() + "\n";
    } else {
        return "if (!translate(in." + field.fullName + ", &out->" + field.field->name() +
               ")) return false;\n";
    }
}

static void h2aScalarChecks(Formatter& out, const FieldWithVersion& field) {
    static const std::map<ScalarType::Kind, size_t> kSignedMaxSize{
            {ScalarType::KIND_UINT8, std::numeric_limits<int8_t>::max()},
            {ScalarType::KIND_INT16, std::numeric_limits<int32_t>::max()},
            {ScalarType::KIND_UINT32, std::numeric_limits<int32_t>::max()},
            {ScalarType::KIND_UINT64, std::numeric_limits<int64_t>::max()}};

    const ScalarType* scalarType = field.field->type().resolveToScalarType();
    if (scalarType != nullptr) {
        const auto& it = kSignedMaxSize.find(scalarType->getKind());
        if (it != kSignedMaxSize.end()) {
            out << "// FIXME This requires conversion between signed and unsigned. Change this if "
                   "it doesn't suit your needs.\n";
            if (scalarType->getKind() == ScalarType::KIND_INT16) {
                // AIDL uses an unsigned 16-bit integer(char16_t), so this is signed to unsigned.
                out << "if (in." << field.fullName << " < 0) return false;\n";
            } else {
                out << "if (in." << field.fullName << " > " << it->second << ") return false;\n";
            }
        }
    }
}

static void fieldTranslation(Formatter& out, const std::set<const NamedType*>& namedTypes,
                             const FieldWithVersion& field) {
    // TODO(b/158489355) Need to support and validate more types like arrays/vectors.
    if (field.field->type().isNamedType()) {
        out << namedTypeTranslation(namedTypes, static_cast<const NamedType*>(field.field->get()),
                                    field);
    } else if (field.field->type().isEnum() || field.field->type().isScalar() ||
               field.field->type().isString()) {
        h2aScalarChecks(out, field);
        out << "out->" << field.field->name() << " = in." << field.fullName << ";\n";
    } else {
        AidlHelper::notes() << "An unhandled type was found in translation: "
                            << field.field->type().typeName() << "\n";
        out << "#error FIXME Unhandled type: " << field.field->type().typeName() << "\n";
    }
}

static const std::string cppAidlTypePackage(const NamedType* type) {
    return "aidl::" +
           base::Join(base::Split(AidlHelper::getAidlPackage(type->fqName()), "."), "::") +
           "::" + AidlHelper::getAidlType(*type, type->fqName());
}

static const std::string declareAidlFunctionSignature(const NamedType* type) {
    return "__attribute__((warn_unused_result)) bool translate(const " + type->fullName() +
           "& in, " + cppAidlTypePackage(type) + "* out)";
}

static const std::string ndkGetPackageFilePath(const NamedType* type) {
    return base::Join(base::Split(type->fqName().package(), "."), "/");
}

static bool typeComesFromInterface(const NamedType* type) {
    const Scope* parent = type->parent();
    while (parent != nullptr) {
        if (parent->isInterface()) {
            return true;
        }
        parent = parent->parent();
    }
    return false;
}

static const std::string ndkHidlIncludeFile(const NamedType* type) {
    if (typeComesFromInterface(type)) {
        return "#include \"" + ndkGetPackageFilePath(type) + "/" + type->fqName().version() + "/" +
               type->parent()->fqName().getInterfaceName() + ".h\"\n";
    } else {
        return "#include \"" + ndkGetPackageFilePath(type) + "/" + type->fqName().version() +
               "/types.h\"\n";
    }
}

static const std::string ndkAidlIncludeFile(const NamedType* type) {
    return "#include \"aidl/" + ndkGetPackageFilePath(type) + "/" +
           AidlHelper::getAidlType(*type, type->fqName()) + ".h\"\n";
}

static void emitNdkTranslateHeader(
        const std::set<const NamedType*>& namedTypes,
        const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes) {
    std::set<std::string> includes;

    AidlHelper::translatorHeader() << "// FIXME: license file if you have one\n\n";
    AidlHelper::translatorHeader() << "#pragma once\n\n";
    for (const auto& type : namedTypes) {
        const auto& it = processedTypes.find(type);
        if (it == processedTypes.end()) {
            continue;
        }
        includes.insert(ndkAidlIncludeFile(type));
        includes.insert(ndkHidlIncludeFile(type));
    }
    AidlHelper::translatorHeader() << base::Join(includes, "") << "\n\n";

    AidlHelper::translatorHeader() << "namespace android::h2a {\n\n";
    for (const auto& type : namedTypes) {
        const auto& it = processedTypes.find(type);
        if (it == processedTypes.end()) {
            continue;
        }
        AidlHelper::translatorHeader() << declareAidlFunctionSignature(type) << ";\n";
    }
    AidlHelper::translatorHeader() << "\n}  // namespace android::h2a\n";
}

static void emitNdkTranslateSource(
        const std::set<const NamedType*>& namedTypes,
        const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes) {
    AidlHelper::translatorSource() << "// FIXME: license file if you have one\n\n";
    AidlHelper::translatorSource()
            << "#include \"" << AidlHelper::getAidlPackagePath((*namedTypes.begin())->fqName())
            << "/translate-ndk.h\"\n\n";
    AidlHelper::translatorSource() << "namespace android::h2a {\n\n";
    for (const auto& type : namedTypes) {
        const auto& it = processedTypes.find(type);
        if (it == processedTypes.end()) {
            continue;
        }
        AidlHelper::translatorSource() << declareAidlFunctionSignature(type) << " {\n";
        AidlHelper::translatorSource().indent([&] {
            const ProcessedCompoundType& processedType = it->second;
            for (const auto& field : processedType.fields) {
                fieldTranslation(AidlHelper::translatorSource(), namedTypes, field);
            }

            AidlHelper::translatorSource() << "return true;\n";
        });
        AidlHelper::translatorSource() << "}\n\n";
    }
    AidlHelper::translatorSource() << "}  // namespace android::h2a";
}

void AidlHelper::emitH2aTranslation(
        const std::set<const NamedType*>& namedTypes,
        const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes) {
    // TODO(b/158489355) Need to support Java and cpp as well
    if (processedTypes.empty()) return;
    emitNdkTranslateHeader(namedTypes, processedTypes);
    emitNdkTranslateSource(namedTypes, processedTypes);
}

}  // namespace android
