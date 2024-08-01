load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "compiledb",
    targets = {
        "//:lifec": "",
    },
)

cc_library(
    name = "life-lib",
    srcs = ["src/lib.cpp"],
    hdrs = ["src/lib.hpp"],
)

cc_binary(
    name = "lifec",
    srcs = ["src/main.cpp"],
    deps = [
        "life-lib",
        "@boost//:algorithm",
    ],
)
