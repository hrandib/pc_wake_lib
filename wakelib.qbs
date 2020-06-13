import qbs 1.0

StaticLibrary {
    name: "wakelib"
    files: [
        "*.cpp",
        "*.h",
    ]
    Depends { name: 'cpp' }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.sourceDirectory]
    }
}
