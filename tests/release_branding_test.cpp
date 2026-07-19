// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::string readFile(const char* path) {
    std::ifstream stream(path, std::ios::in | std::ios::binary);
    if (!stream) {
        expect(false, std::string("could not read ") + path);
        return std::string();
    }
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

std::string normalizeWhitespace(const std::string& text) {
    std::string normalized;
    bool pendingSpace = false;
    for (std::string::const_iterator character = text.begin();
         character != text.end(); ++character) {
        const unsigned char value = static_cast<unsigned char>(*character);
        if (std::isspace(value)) {
            pendingSpace = !normalized.empty();
        } else {
            if (pendingSpace)
                normalized.push_back(' ');
            normalized.push_back(*character);
            pendingSpace = false;
        }
    }
    return normalized;
}

std::string asciiLower(const std::string& text) {
    std::string lowered(text);
    for (std::string::iterator character = lowered.begin();
         character != lowered.end(); ++character) {
        const unsigned char value = static_cast<unsigned char>(*character);
        if (value >= static_cast<unsigned char>('A')
            && value <= static_cast<unsigned char>('Z')) {
            *character = static_cast<char>(value - 'A' + 'a');
        }
    }
    return lowered;
}

void expectContains(const std::string& text, const std::string& expected,
                    const std::string& context) {
    expect(text.find(expected) != std::string::npos,
           context + " must contain the approved value");
}

void expectAbsent(const std::string& text, const std::string& forbidden,
                  const std::string& context) {
    expect(text.find(forbidden) == std::string::npos,
           context + " contains forbidden release branding");
}

} // namespace

int main() {
    const std::string expectedAttribution = normalizeWhitespace(
        "Burl is an independently developed digital instrument that recreates "
        "selected observable behaviours of the Benjolin, a 2009 DIY/workshop "
        "instrument designed by Rob Hordijk. The reference architecture combines "
        "two wide-range oscillators, a voltage-controlled filter, and Hordijk’s "
        "feedback shift-register pattern generator, called the Rungler. This "
        "project is not affiliated with, sponsored by, approved by, or endorsed "
        "by the original designer’s successors, After Later Audio, Epoch Modular, "
        "or Macumbista. Third-party names are used only to identify the historical "
        "instrument studied. The MIT License applies only to this project’s "
        "original software and files expressly covered by its license notice.");

    const std::string readme = readFile("README.md");
    const std::string releaseGate = readFile("docs/RELEASE_GATE.md");
    const std::string thirdPartyNotices = readFile("THIRD_PARTY_NOTICES.md");
    const std::string pluginSource = readFile("src/plugin.cpp");
    const std::string makefile = readFile("Makefile");
    const std::string pluginObject = readFile("plugins/Burl.o");

    expect(readme.compare(0u, 7u, "# Burl\n") == 0,
           "README must use Burl as its primary product heading");
    expectContains(normalizeWhitespace(readme), expectedAttribution,
                   "README release attribution");
    expectContains(normalizeWhitespace(releaseGate), expectedAttribution,
                   "release-gate attribution record");

    expectContains(readme, "**Version:** 1.0.0", "release version");
    expectContains(readme, "**Author:** Neal Sanche", "release author");
    expectContains(readme, "https://github.com/thorinside/burl",
                   "release homepage");
    expectContains(readme, "`disting-nt`", "release tags");
    expectContains(releaseGate,
                   "7def2805849cd98dc77b7ee6d860e054db54b1b5",
                   "release source commit");
    expectContains(releaseGate,
                   "cd12d876dbe060859828053efab1cbc98c9df251",
                   "pinned API dependency");
    expectContains(thirdPartyNotices, "Copyright (c) 2025 Expert Sleepers Ltd",
                   "API dependency copyright");
    expectContains(thirdPartyNotices, "MIT License",
                   "API dependency license");

    const std::string normalizedGate = normalizeWhitespace(releaseGate);
    expectContains(normalizedGate, "**Public name:** Burl.",
                   "release-name decision");
    expectContains(normalizedGate,
                   "the existing U.S. BURL wordmark is accepted as a known, low "
                   "release risk rather than a blocker",
                   "accepted naming-risk record");
    expectContains(normalizedGate, "no claim of trademark rights",
                   "trademark-rights posture");
    expectContains(normalizedGate,
                   "willingness to rename after a credible conflict or objection",
                   "rename posture");

    expectContains(pluginSource, "NT_drawText(0, 18, \"Burl\");",
                   "on-device title");
    expectContains(pluginSource, "\"Burl\",", "factory name");
    expectContains(makefile, "PLUGIN_OUTPUT := plugins/Burl.o",
                   "installable object name");
    expectContains(pluginObject, "Burl", "installable object metadata");

    const char* implementationFiles[] = {
        "include/burl/filter.hpp",
        "include/burl/pattern_generator.hpp",
        "include/burl/voice.hpp",
        "src/filter.cpp",
        "src/pattern_generator.cpp",
        "src/plugin.cpp",
        "src/voice.cpp"
    };
    for (std::size_t index = 0u;
         index < sizeof(implementationFiles) / sizeof(implementationFiles[0]);
         ++index) {
        const std::string lowered = asciiLower(readFile(implementationFiles[index]));
        expectAbsent(lowered, "benji", implementationFiles[index]);
        expectAbsent(lowered, "benjolin", implementationFiles[index]);
        expectAbsent(lowered, "rungler", implementationFiles[index]);
    }

    const std::string loweredObject = asciiLower(pluginObject);
    expectAbsent(loweredObject, "benji", "installable object");
    expectAbsent(loweredObject, "benjolin", "installable object");
    expectAbsent(loweredObject, "rungler", "installable object");

    const std::string trademarkMark("\xe2\x84\xa2", 3u);
    const std::string registeredMark("\xc2\xae", 2u);
    const char* releaseBrandSurfaces[] = {
        "README.md",
        "LICENSE",
        "Makefile",
        "include/burl/filter.hpp",
        "include/burl/pattern_generator.hpp",
        "include/burl/voice.hpp",
        "src/filter.cpp",
        "src/pattern_generator.cpp",
        "src/plugin.cpp",
        "src/voice.cpp",
        "plugins/Burl.o"
    };
    for (std::size_t index = 0u;
         index < sizeof(releaseBrandSurfaces) / sizeof(releaseBrandSurfaces[0]);
         ++index) {
        const std::string contents = readFile(releaseBrandSurfaces[index]);
        expectAbsent(contents, trademarkMark, releaseBrandSurfaces[index]);
        expectAbsent(contents, registeredMark, releaseBrandSurfaces[index]);
    }

    if (failures != 0)
        return 1;
    std::cout << "Burl release branding and attribution checks passed\n";
    return 0;
}
