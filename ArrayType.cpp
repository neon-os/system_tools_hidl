#include "ArrayType.h"

#include "Formatter.h"

#include <android-base/logging.h>

namespace android {

ArrayType::ArrayType(Type *elementType, const char *dimension)
    : mElementType(elementType),
      mDimension(dimension) {
}

std::string ArrayType::getCppType(StorageMode mode, std::string *extra) const {
    const std::string base = mElementType->getCppType(extra);

    CHECK(extra->empty());

    *extra = "[" + mDimension + "]";

    switch (mode) {
        case StorageMode_Stack:
            return base;

        case StorageMode_Argument:
            return "const " + base;

        case StorageMode_Result:
        {
            extra->clear();
            return "const " + base + "*";
        }
    }
}

void ArrayType::emitReaderWriter(
        Formatter &out,
        const std::string &name,
        const std::string &parcelObj,
        bool parcelObjIsPointer,
        bool isReader,
        ErrorMode mode) const {
    std::string baseExtra;
    std::string baseType = mElementType->getCppType(&baseExtra);

    const std::string parentName = "_aidl_" + name + "_parent";

    out << "size_t " << parentName << ";\n\n";

    const std::string parcelObjDeref =
        parcelObj + (parcelObjIsPointer ? "->" : ".");

    if (isReader) {
        out << name
            << " = (const "
            << baseType
            << " *)"
            << parcelObjDeref
            << "readBuffer(&"
            << parentName
            << ");\n\n";

        out << "if (" << name << " == nullptr) {\n";

        out.indent();

        out << "_aidl_err = ::android::UNKNOWN_ERROR;\n";
        handleError2(out, mode);

        out.unindent();
        out << "}\n\n";
    } else {
        out << "_aidl_err = "
            << parcelObjDeref
            << "writeBuffer("
            << name
            << ", "
            << mDimension
            << " * sizeof("
            << baseType
            << "), &"
            << parentName
            << ");\n";

        handleError(out, mode);
    }

    emitReaderWriterEmbedded(
            out,
            name,
            isReader /* nameIsPointer */,
            parcelObj,
            parcelObjIsPointer,
            isReader,
            mode,
            parentName,
            "0 /* parentOffset */");
}

void ArrayType::emitReaderWriterEmbedded(
        Formatter &out,
        const std::string &name,
        bool nameIsPointer,
        const std::string &parcelObj,
        bool parcelObjIsPointer,
        bool isReader,
        ErrorMode mode,
        const std::string &parentName,
        const std::string &offsetText) const {
    if (!mElementType->needsEmbeddedReadWrite()) {
        return;
    }

    const std::string nameDeref = name + (nameIsPointer ? "->" : ".");

    std::string baseExtra;
    std::string baseType = mElementType->getCppType(&baseExtra);

    out << "for (size_t _aidl_index = 0; _aidl_index < "
        << mDimension
        << "; ++_aidl_index) {\n";

    out.indent();

    mElementType->emitReaderWriterEmbedded(
            out,
            name + "[_aidl_index]",
            false /* nameIsPointer */,
            parcelObj,
            parcelObjIsPointer,
            isReader,
            mode,
            parentName,
            offsetText + " + _aidl_index * sizeof(" + baseType + ")");

    out.unindent();

    out << "}\n\n";
}

bool ArrayType::needsEmbeddedReadWrite() const {
    return mElementType->needsEmbeddedReadWrite();
}

}  // namespace android
