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

#include "AidlHelper.h"
#include "CompoundType.h"
#include "Coordinator.h"
#include "EnumType.h"
#include "NamedType.h"
#include "TypeDef.h"

namespace android {

struct FieldWithVersion {
    const NamedReference<Type>* field;
    std::pair<size_t, size_t> version;
};

struct ProcessedCompoundType {
    std::vector<FieldWithVersion> fields;
    std::set<const NamedType*> subTypes;
};

static void emitConversionNotes(Formatter& out, const NamedType& namedType) {
    out << "// This is the HIDL definition of " << namedType.fqName().string() << "\n";
    out.pushLinePrefix("// ");
    namedType.emitHidlDefinition(out);
    out.popLinePrefix();
    out << "\n";
}

static void emitTypeDefAidlDefinition(Formatter& out, const TypeDef& typeDef) {
    out << "// Cannot convert typedef " << typeDef.referencedType()->definedName() << " "
        << typeDef.fqName().string() << " since AIDL does not support typedefs.\n";
    emitConversionNotes(out, typeDef);
}

static void emitEnumAidlDefinition(Formatter& out, const EnumType& enumType) {
    const ScalarType* scalar = enumType.storageType()->resolveToScalarType();
    CHECK(scalar != nullptr) << enumType.typeName();

    enumType.emitDocComment(out);
    out << "@Backing(type=\"" << AidlHelper::getAidlType(*scalar, enumType.fqName()) << "\")\n";
    out << "enum " << enumType.fqName().name() << " ";
    out.block([&] {
        enumType.forEachValueFromRoot([&](const EnumValue* value) {
            value->emitDocComment(out);
            out << value->name();
            if (!value->isAutoFill()) {
                out << " = " << value->constExpr()->expression();
            }
            out << ",\n";
        });
    });
}

void processCompoundType(const CompoundType& compoundType, ProcessedCompoundType* processedType) {
    // Gather all of the subtypes defined in this type
    for (const NamedType* subType : compoundType.getSubTypes()) {
        processedType->subTypes.insert(subType);
    }
    std::pair<size_t, size_t> version = compoundType.fqName().hasVersion()
                                                ? compoundType.fqName().getVersion()
                                                : std::pair<size_t, size_t>{0, 0};
    for (const NamedReference<Type>* field : compoundType.getFields()) {
        // Check for references to an older version of itself
        if (field->get()->typeName() == compoundType.typeName()) {
            processCompoundType(static_cast<const CompoundType&>(*field->get()), processedType);
        } else {
            // Handle duplicate field names. Keep only the most recent definitions.
            auto it = std::find_if(processedType->fields.begin(), processedType->fields.end(),
                                   [field](auto& processedField) {
                                       return processedField.field->name() == field->name();
                                   });
            if (it != processedType->fields.end()) {
                AidlHelper::notes()
                        << "Found conflicting field name \"" << field->name()
                        << "\" in different versions of " << compoundType.fqName().name() << ". ";

                if (version.first > it->version.first ||
                    (version.first == it->version.first && version.second > it->version.second)) {
                    AidlHelper::notes()
                            << "Keeping " << field->get()->typeName() << " from " << version.first
                            << "." << version.second << " and discarding "
                            << (it->field)->get()->typeName() << " from " << it->version.first
                            << "." << it->version.second << ".\n";

                    it->field = field;
                    it->version = version;
                } else {
                    AidlHelper::notes()
                            << "Keeping " << (it->field)->get()->typeName() << " from "
                            << it->version.first << "." << it->version.second << " and discarding "
                            << field->get()->typeName() << " from " << version.first << "."
                            << version.second << ".\n";
                }
            } else {
                processedType->fields.push_back({field, version});
            }
        }
    }
}

static void emitCompoundTypeAidlDefinition(Formatter& out, const CompoundType& compoundType,
                                           const Coordinator& coordinator) {
    // Get all of the subtypes and fields from this type and any older versions
    // that it references.
    ProcessedCompoundType processedType;
    processCompoundType(compoundType, &processedType);

    // Emit all of the subtypes
    for (const NamedType* namedType : processedType.subTypes) {
        AidlHelper::emitAidl(*namedType, coordinator);
    }

    // Add all of the necessary imports for types that were found in older versions and missed
    // when emitting the file header.
    std::set<std::string> imports;
    const std::vector<const NamedReference<Type>*>& latestFields = compoundType.getFields();
    const std::vector<NamedType*>& latestSubTypes = compoundType.getSubTypes();
    for (auto const& fieldWithVersion : processedType.fields) {
        if (std::find(latestFields.begin(), latestFields.end(), fieldWithVersion.field) ==
            latestFields.end()) {
            AidlHelper::importLocallyReferencedType(*fieldWithVersion.field->get(), &imports);
        }
    }
    for (const NamedType* subType : processedType.subTypes) {
        if (std::find(latestSubTypes.begin(), latestSubTypes.end(), subType) ==
            latestSubTypes.end()) {
            AidlHelper::importLocallyReferencedType(*subType, &imports);
        }
    }
    for (const std::string& import : imports) {
        out << "import " << import << ";\n";
    }
    if (imports.size() > 0) {
        out << "\n";
    }

    compoundType.emitDocComment(out);
    out << "parcelable " << AidlHelper::getAidlName(compoundType.fqName()) << " ";
    if (compoundType.style() == CompoundType::STYLE_STRUCT) {
        out.block([&] {
            // Emit all of the fields from the processed type
            for (auto const& fieldWithVersion : processedType.fields) {
                fieldWithVersion.field->emitDocComment(out);
                out << AidlHelper::getAidlType(*fieldWithVersion.field->get(),
                                               compoundType.fqName())
                    << " " << fieldWithVersion.field->name() << ";\n";
            }
        });
    } else {
        out << "{}\n";
        out << "// Cannot convert unions/safe_unions since AIDL does not support them.\n";
        emitConversionNotes(out, compoundType);
    }
    out << "\n\n";
}

// TODO: Enum/Typedef should just emit to hidl-error.log or similar
void AidlHelper::emitAidl(const NamedType& namedType, const Coordinator& coordinator) {
    Formatter out = getFileWithHeader(namedType, coordinator);
    if (namedType.isTypeDef()) {
        const TypeDef& typeDef = static_cast<const TypeDef&>(namedType);
        emitTypeDefAidlDefinition(out, typeDef);
    } else if (namedType.isCompoundType()) {
        const CompoundType& compoundType = static_cast<const CompoundType&>(namedType);
        emitCompoundTypeAidlDefinition(out, compoundType, coordinator);
    } else if (namedType.isEnum()) {
        const EnumType& enumType = static_cast<const EnumType&>(namedType);
        emitEnumAidlDefinition(out, enumType);
    } else {
        out << "// TODO: Fix this " << namedType.definedName() << "\n";
    }
}

}  // namespace android
