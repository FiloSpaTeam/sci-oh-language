#include "scioh/Codegen.hpp"
#include "scioh/Diagnostic.hpp"
#include "scioh/Lexer.hpp"
#include "scioh/Parser.hpp"

#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct Options {
    bool emitCpp = false;
    std::string inputPath;
    std::string outputPath;
};

constexpr std::string_view kVersion = "0.1.0";
constexpr std::string_view kMotto = "Lu compilatore che compila quann ie pare";

void printUsage(std::ostream& out) {
    out << "uso: sci-oh [--emit-cpp] [--version] <file.sci> [-o output]\n";
}

void printVersion(std::ostream& out) {
    out << "sci-oh " << kVersion << '\n';
    out << kMotto << '\n';
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("ne riesche a apre " + path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const std::filesystem::path& path, std::string_view content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("ne riesche a scrive " + path.string());
    }

    file << content;
}

std::string shellQuote(const std::filesystem::path& path) {
    std::string value = path.string();
    std::string quoted = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

Options parseOptions(int argc, char** argv) {
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--help" || arg == "-h") {
            printUsage(std::cout);
            std::exit(0);
        }

        if (arg == "--version" || arg == "-v") {
            printVersion(std::cout);
            std::exit(0);
        }

        if (arg == "--emit-cpp") {
            options.emitCpp = true;
            continue;
        }

        if (arg == "-o") {
            if (index + 1 >= argc) {
                throw std::runtime_error("manca lu nome d'uscita pe -o");
            }
            options.outputPath = argv[++index];
            continue;
        }

        if (!options.inputPath.empty()) {
            throw std::runtime_error("troppi file da compila': " + arg);
        }
        options.inputPath = arg;
    }

    if (options.inputPath.empty()) {
        throw std::runtime_error("manca lu file sorgente");
    }

    return options;
}

std::string compileToCpp(const std::string& source) {
    scioh::Lexer lexer(source);
    scioh::Parser parser(std::move(lexer));
    auto program = parser.parseProgram();

    std::ostringstream out;
    scioh::Codegen codegen;
    codegen.emit(program, out);
    return out.str();
}

std::filesystem::path temporaryCppPath() {
    auto path = std::filesystem::temp_directory_path();
    path /= "sci-oh-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".cpp";
    return path;
}

int buildExecutable(const std::string& cpp, const std::string& outputPath) {
    const auto cppPath = temporaryCppPath();
    writeFile(cppPath, cpp);

    const char* compiler = std::getenv("CXX");
    if (compiler == nullptr || std::string_view(compiler).empty()) {
        compiler = "c++";
    }

    const std::filesystem::path output = outputPath.empty() ? "a.out" : outputPath;
    const std::string command = std::string(compiler) + " -std=c++17 -O2 " + shellQuote(cppPath) + " -o " + shellQuote(output);
    const int result = std::system(command.c_str());
    std::filesystem::remove(cppPath);
    return result;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parseOptions(argc, argv);
        const auto source = readFile(options.inputPath);
        const auto cpp = compileToCpp(source);

        if (options.emitCpp) {
            if (options.outputPath.empty()) {
                std::cout << cpp;
            } else {
                writeFile(options.outputPath, cpp);
            }
            return 0;
        }

        const int result = buildExecutable(cpp, options.outputPath);
        if (result != 0) {
            std::cerr << "errore: lu C++ s'e' 'nciampate\n";
            return 1;
        }
        return 0;
    } catch (const scioh::DiagnosticError& error) {
        std::cerr << "errore: " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "errore: " << error.what() << '\n';
        printUsage(std::cerr);
        return 1;
    }
}
