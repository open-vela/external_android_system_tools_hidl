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
#include "Coordinator.h"
#include "NamedType.h"
#include "ScalarType.h"
#include "Scope.h"

namespace android {

std::string AidlHelper::translateHeaderFile(const FQName& fqName, AidlBackend backend) {
    switch (backend) {
        case AidlBackend::NDK:
            return AidlHelper::getAidlPackagePath(fqName) + "/translate-ndk.h";
        case AidlBackend::CPP:
            return AidlHelper::getAidlPackagePath(fqName) + "/translate-cpp.h";
        default:
            LOG(FATAL) << "Unexpected AidlBackend value";
            return "";
    }
}

std::string AidlHelper::translateSourceFile(const FQName& fqName, AidlBackend backend) {
    switch (backend) {
        case AidlBackend::NDK:
            return AidlHelper::getAidlPackagePath(fqName) + "/translate-ndk.cpp";
        case AidlBackend::CPP:
            return AidlHelper::getAidlPackagePath(fqName) + "/translate-cpp.cpp";
        default:
            LOG(FATAL) << "Unexpected AidlBackend value";
            return "";
    }
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

static void cppH2aFieldTranslation(Formatter& out, const std::set<const NamedType*>& namedTypes,
                                   const FieldWithVersion& field, AidlBackend backend) {
    // TODO(b/158489355) Need to support and validate more types like arrays/vectors.
    if (field.field->type().isNamedType()) {
        out << namedTypeTranslation(namedTypes, static_cast<const NamedType*>(field.field->get()),
                                    field);
    } else if (field.field->type().isEnum() || field.field->type().isScalar()) {
        h2aScalarChecks(out, field);
        out << "out->" << field.field->name() << " = in." << field.fullName << ";\n";
    } else if (field.field->type().isString()) {
        if (backend == AidlBackend::CPP) {
            out << "// FIXME Need to make sure the hidl_string is valid utf-8, otherwise an empty "
                   "String16 will be returned.\n";
            out << "out->" << field.field->name() << " = String16(in." << field.fullName
                << ".c_str());\n";
        } else {
            out << "out->" << field.field->name() << " = in." << field.fullName << ";\n";
        }
    } else {
        AidlHelper::notes() << "An unhandled type was found in translation: "
                            << field.field->type().typeName() << "\n";
        out << "#error FIXME Unhandled type: " << field.field->type().typeName() << "\n";
    }
}

static const std::string cppAidlTypePackage(const NamedType* type, AidlBackend backend) {
    const std::string prefix = (backend == AidlBackend::NDK) ? "aidl::" : std::string();
    return prefix + base::Join(base::Split(AidlHelper::getAidlPackage(type->fqName()), "."), "::") +
           "::" + AidlHelper::getAidlType(*type, type->fqName());
}

static const std::string cppDeclareAidlFunctionSignature(const NamedType* type,
                                                         AidlBackend backend) {
    return "__attribute__((warn_unused_result)) bool translate(const " + type->fullName() +
           "& in, " + cppAidlTypePackage(type, backend) + "* out)";
}

static const std::string getPackageFilePath(const NamedType* type) {
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

static const std::string hidlIncludeFile(const NamedType* type) {
    if (typeComesFromInterface(type)) {
        return "#include \"" + getPackageFilePath(type) + "/" + type->fqName().version() + "/" +
               type->parent()->fqName().getInterfaceName() + ".h\"\n";
    } else {
        return "#include \"" + getPackageFilePath(type) + "/" + type->fqName().version() +
               "/types.h\"\n";
    }
}

static const std::string aidlIncludeFile(const NamedType* type, AidlBackend backend) {
    const std::string prefix = (backend == AidlBackend::NDK) ? "aidl/" : std::string();
    return "#include \"" + prefix + getPackageFilePath(type) + "/" +
           AidlHelper::getAidlType(*type, type->fqName()) + ".h\"\n";
}

static void emitCppTranslateHeader(
        const Coordinator& coordinator, const FQName& fqName,
        const std::set<const NamedType*>& namedTypes,
        const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes,
        AidlBackend backend) {
    CHECK(backend == AidlBackend::CPP || backend == AidlBackend::NDK);
    std::set<std::string> includes;
    Formatter out =
            coordinator.getFormatter(fqName, Coordinator::Location::DIRECT,
                                     "include/" + AidlHelper::translateHeaderFile(fqName, backend));

    AidlHelper::emitFileHeader(out);
    out << "#pragma once\n\n";
    for (const auto& type : namedTypes) {
        const auto& it = processedTypes.find(type);
        if (it == processedTypes.end()) {
            continue;
        }
        includes.insert(aidlIncludeFile(type, backend));
        includes.insert(hidlIncludeFile(type));
    }
    out << base::Join(includes, "") << "\n\n";

    out << "namespace android::h2a {\n\n";
    for (const auto& type : namedTypes) {
        const auto& it = processedTypes.find(type);
        if (it == processedTypes.end()) {
            continue;
        }
        out << cppDeclareAidlFunctionSignature(type, backend) << ";\n";
    }
    out << "\n}  // namespace android::h2a\n";
}

static void emitCppTranslateSource(
        const Coordinator& coordinator, const FQName& fqName,
        const std::set<const NamedType*>& namedTypes,
        const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes,
        AidlBackend backend) {
    CHECK(backend == AidlBackend::CPP || backend == AidlBackend::NDK);
    Formatter out = coordinator.getFormatter(fqName, Coordinator::Location::DIRECT,
                                             AidlHelper::translateSourceFile(fqName, backend));
    AidlHelper::emitFileHeader(out);
    out << "#include \""
        << AidlHelper::translateHeaderFile((*namedTypes.begin())->fqName(), backend) + "\"\n\n";
    out << "namespace android::h2a {\n\n";
    for (const auto& type : namedTypes) {
        const auto& it = processedTypes.find(type);
        if (it == processedTypes.end()) {
            continue;
        }
        out << cppDeclareAidlFunctionSignature(type, backend) << " {\n";
        out.indent([&] {
            const ProcessedCompoundType& processedType = it->second;
            for (const auto& field : processedType.fields) {
                cppH2aFieldTranslation(out, namedTypes, field, backend);
            }
            out << "return true;\n";
        });
        out << "}\n\n";
    }
    out << "}  // namespace android::h2a";
}

void AidlHelper::emitTranslation(
        const Coordinator& coordinator, const FQName& fqName,
        const std::set<const NamedType*>& namedTypes,
        const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes) {
    if (processedTypes.empty()) return;
    for (auto backend : {AidlBackend::NDK, AidlBackend::CPP}) {
        emitCppTranslateHeader(coordinator, fqName, namedTypes, processedTypes, backend);
        emitCppTranslateSource(coordinator, fqName, namedTypes, processedTypes, backend);
    }
}

}  // namespace android
