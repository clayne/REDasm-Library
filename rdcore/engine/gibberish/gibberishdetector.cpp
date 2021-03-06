#include "gibberishdetector.h"
#include <iostream>
#include <numeric>
#include <cassert>
#include <cmath>

const size_t GibberishDetector::DEFAULT_COUNTS_VALUE = 10;
GibberishDetectorData::MatrixCounts GibberishDetector::m_counts;
GibberishDetector::CharIndex GibberishDetector::m_charidx;

bool GibberishDetector::train(const std::string& bigfile, const std::string& goodfile, const std::string& badfile)
{
    GibberishDetector::initializeCounts();
    GibberishDetector::initializeCharIndex();

    bool res = GibberishDetector::lines(bigfile, [](const std::string& line) {
        for(const auto& [a, b] : GibberishDetector::ngram(line))
            m_counts[m_charidx[a]][m_charidx[b]] += 1;
    });

    if(!res) return false;

    for(auto& row : m_counts)
    {
        double s = static_cast<double>(std::accumulate(row.begin(), row.end(), 0));

        std::transform(row.begin(), row.end(), row.begin(), [s](double cell) -> double {
            return std::log(cell / s);
        });
    }

    std::deque<double> goodprobs, badprobs;

    res = GibberishDetector::lines(goodfile, [&](const std::string& line) { goodprobs.push_back(GibberishDetector::avgTransitionProb(line, m_counts)); });
    if(!res) return false;

    res = GibberishDetector::lines(badfile, [&](const std::string& line) { badprobs.push_back(GibberishDetector::avgTransitionProb(line, m_counts)); });
    if(!res) return false;

    assert(goodprobs.size() > badprobs.size());
    double min = *std::min_element(goodprobs.begin(), goodprobs.end());
    double max = *std::max_element(badprobs.begin(), badprobs.end());

    double thresh = (min + max) / 2.0;
    std::cout << "const double TRAINED_THRESHOLD = " << thresh << "; " << std::endl;
    std::cout << "const double TRAINED_COUNTS = {\n{" << std::endl;

    for(const auto& row : m_counts)
    {
        std::cout << "{";

        for(const auto& col : row)
        {
            std::cout << col;
            if(col != row.back()) std::cout << ", ";
        }

        std::cout << "}";
        if(row != m_counts.back()) std::cout << ",";
        std::cout << std::endl;
    }

    std::cout << "}\n};" << std::endl;
    return true;
}

bool GibberishDetector::isGibberish(const std::string& s)
{
    return GibberishDetector::avgTransitionProb(s, GibberishDetectorData::TRAINED_COUNTS) <= GibberishDetectorData::TRAINED_THRESHOLD;
}

GibberishDetector::NGramList GibberishDetector::ngram(const std::string& s)
{
    std::string ns = GibberishDetector::normalize(s);
    NGramList nglist;

    for(int i = 0; i < static_cast<int>(ns.size()) - 1; i++)
        nglist.push_back({ ns[i], ns[i + 1] });

    return nglist;
}

void GibberishDetector::initializeCounts()
{
    m_counts = GibberishDetector::fillMatrix<GibberishDetectorData::MatrixCounts::value_type::value_type, m_counts.size()>(DEFAULT_COUNTS_VALUE);
}

std::string GibberishDetector::normalize(std::string s)
{
    for(auto it = s.begin(); it != s.end(); ) // Remove [^a-z ]+
    {
        if((*it == ' ') || ::isalpha(*it))
        {
            *it = ::tolower(*it);
            it++;
        }
        else
            it = s.erase(it);
    }

    return s;
}

double GibberishDetector::avgTransitionProb(const std::string& s, const GibberishDetectorData::MatrixCounts& counts)
{
    double logprob = 0;
    int transitionct = 0;

    for(const auto& n : GibberishDetector::ngram(s))
    {
        logprob += counts[m_charidx[n.first]][m_charidx[n.second]];
        transitionct += 1;
    }

    return std::exp(logprob / (transitionct ? transitionct : 1));
}

void GibberishDetector::initialize() { GibberishDetector::initializeCharIndex(); }

void GibberishDetector::initializeCharIndex()
{
    std::for_each(GibberishDetectorData::ACCEPTED_CHARS.begin(), GibberishDetectorData::ACCEPTED_CHARS.end(), [](char c) {
        m_charidx.emplace(c, m_charidx.size());
    });
}
