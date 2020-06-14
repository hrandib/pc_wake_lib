import qbs 1.0

StaticLibrary {
    name: "wakelib"

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
    Group { name: "win"
        condition: qbs.targetOS.contains("windows")
        files: [
            "serialport_win.h",
            "serialport_win.cpp"
        ]
    }
    Group { name: "linux"
        condition: qbs.targetOS.contains("linux")
        files: [
            "serialport_linux.h",
            "serialport_linux.cpp"
        ]
    }

    Depends { name: 'cpp' }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [ product.sourceDirectory ]
    }
}
