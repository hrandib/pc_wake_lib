import qbs 1.0
import qbs.FileInfo

StaticLibrary {
    name: "wake"

    readonly property string PlatformPath:
        qbs.targetOS.contains("windows") ? "win/" : "linux/"

    cpp.includePaths: [
        sourceDirectory
    ]

    cpp.defines: [
        //"DEBUG_MODE"
    ]

    Group { name: "include"
        files: [
            "iserialport.h",
            "crc8.h",
            "utils.h",
            "option_parser.h",
            "wsp32.h",
        ]
    }

    Group { name: "source"
        files: [
            "crc8.cpp",
            "wsp32.cpp",
        ]
    }

    Group { name: "serialport"
        prefix: PlatformPath
        files: [
            "serialport.h",
            "serialport.cpp"
        ]
    }

    Depends { name: 'cpp' }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            product.sourceDirectory,
            FileInfo.joinPaths(product.sourceDirectory,
                               product.PlatformPath)
        ]
    }
}
