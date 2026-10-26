#include "../interface/DYCoreParameters.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

std::ifstream openCsv(const char *fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        file.clear();
        file.open(std::string("DYshapes/") + fileName);
    }
    if (!file.is_open()) {
        file.clear();
        file.open(std::string("Background/DYshapes/") + fileName);
    }
    return file;
}

bool nextRow(std::ifstream &file, const std::string &cat,
             std::vector<double> &values) {
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream row(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(row, field, ','))
            fields.push_back(field);
        if (fields.size() < 2 || fields[0] != cat)
            continue;

        values.clear();
        for (std::size_t i = 1; i < fields.size(); ++i) {
            try {
                values.push_back(std::stod(fields[i]));
            } catch (const std::exception &) {
                throw std::runtime_error(
                    "Invalid numeric value in DY core CSV row for " + cat);
            }
        }
        return true;
    }
    return false;
}

} // namespace

DYExpParameters loadDYExpParameters(const std::string &cat) {
    std::ifstream file = openCsv("fit_results_2Exp.csv");
    std::vector<double> values;
    if (!file.is_open() || !nextRow(file, cat, values) || values.size() < 9) {
        throw std::runtime_error(
            "Could not find valid 2Exp parameters for category " + cat);
    }
    return {values[0], values[2], values[4], values[6], values[8]};
}

DYPowParameters loadDYPowParameters(const std::string &cat) {
    std::ifstream file = openCsv("fit_results_2PowerLaw.csv");
    std::vector<double> values;
    if (!file.is_open() || !nextRow(file, cat, values) || values.size() < 9) {
        throw std::runtime_error(
            "Could not find valid 2PowerLaw parameters for category " + cat);
    }
    return {values[0], values[2], values[4], values[6], values[8]};
}

DYChebModParameters loadDYCoreModParameters(const std::string &cat) {
    std::ifstream file = openCsv("fit_results_DYCoreMod.csv");
    std::vector<double> values;
    if (!file.is_open() || !nextRow(file, cat, values) || values.size() < 5) {
        throw std::runtime_error(
            "Could not find valid DYCoreMod parameters for category " + cat);
    }
    return {values[0], values[2], values[4]};
}

DYChebModParameters loadDYSplineCoreModParameters(const std::string &cat) {
    std::ifstream file = openCsv("fit_results_DYSplineCoreMod.csv");
    std::vector<double> values;
    if (!file.is_open() || !nextRow(file, cat, values) || values.size() < 5) {
        throw std::runtime_error(
            "Could not find valid DYSplineCoreMod parameters for category " +
            cat);
    }
    return {values[0], values[2], values[4]};
}
