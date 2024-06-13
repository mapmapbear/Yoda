add_rules("mode.debug", "mode.release")
if is_mode("debug") then
    add_defines("DEBUG")
end 
add_ldflags("/subsystem:windows")
package("glfw")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), "external/glfw"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("spdlog")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), "external/spdlog"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("directx_headers")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), "external/directx_headers"))
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

add_requires("spdlog")
add_requires("glfw")
add_requires("directx_headers")

target("sandbox")
    set_languages("c++20")
    add_files("src/application/*.cpp")
    add_files("src/core/*.cpp")
    add_files("src/rhi/d3d12/*.cpp")
    add_files("src/main.cpp")
    add_includedirs("src")
    add_includedirs("external/glfw/include")
    add_includedirs("external/spdlog/include")
    add_includedirs("external/directx_headers/include/directx")
    add_packages("glfw")
    add_packages("spdlog")
    add_packages("directx_headers")
    if is_plat("windows") then
        add_syslinks("user32", "kernel32", "gdi32", "shell32", "dxgi", "d3d12")
    end