/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2016                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <ghoul/logging/logmanager.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/filesystem/directory.h>
#include <openspace/util/time.h>
#include <openspace/util/spicemanager.h>
#include <modules/newhorizons/util/decoder.h>
#include <fstream>
#include <string>
#include <iterator>
#include <iomanip>
#include <limits>
#include <sstream>
#include <modules/newhorizons/util/instrumenttimesparser.h>

namespace {
    const std::string _loggerCat = "InstrumentTimesParser";

    const std::string PlaybookIdentifierName = "InstrumentTimesParser";
}

namespace openspace {

InstrumentTimesParser::InstrumentTimesParser(
    const std::string& name,
    const std::string& sequenceSource,
    ghoul::Dictionary& inputDict)
    : _name(name)
    , _fileName(sequenceSource) 
    , _pattern("\"(.{23})\" \"(.{23})\"")
    , _target("")
{
    ghoul::Dictionary target;
    if (inputDict.getValue("Target", target)) {
        target.getValue("Body", _target);
    }

    ghoul::Dictionary instruments;
    ghoul_assert(inputDict.getValue("Instruments", instruments), "Must define instruments");
    

    _detectorType = "CAMERA"; //default value
        
    

    for (const auto& instrumentKey : instruments.keys()) {
        ghoul::Dictionary instrumentDict; 
        instruments.getValue(instrumentKey, instrumentDict);
        
        _fileTranslation[instrumentKey] = Decoder::createFromDictionary(instrumentDict, "Instrument");

        ghoul::Dictionary files;
        if (instrumentDict.getValue("Files", files)) {
            for (int i = 0; i < files.size(); i++) {
                std::string id = std::to_string(i+1); // lua index starts at 1
                std::string filename;
                if (files.getValue(id, filename)) {
                    _instrumentFiles[instrumentKey].push_back(filename);
                }
            }
        }
    }
}



bool InstrumentTimesParser::create() {
    auto imageComparer = [](const Image &a, const Image &b)->bool{
        return a.startTime < b.startTime;
    };
    auto targetComparer = [](const std::pair<double, std::string> &a,
        const std::pair<double, std::string> &b)->bool{
        return a.first < b.first;
    };
    
    using RawPath = ghoul::filesystem::Directory::RawPath;
    ghoul::filesystem::Directory sequenceDir(_fileName, RawPath::Yes);
    if (!FileSys.directoryExists(sequenceDir)) {
        LERROR("Could not load Label Directory '" << sequenceDir.path() << "'");
        return false;
    }

    using Recursive = ghoul::filesystem::Directory::Recursive;
    using Sort = ghoul::filesystem::Directory::Sort;
    std::vector<std::string> sequencePaths = sequenceDir.read(Recursive::Yes, Sort::No);

    for (auto it = _instrumentFiles.begin(); it != _instrumentFiles.end(); it++) {
        std::string instrumentID = it->first;
        for (std::string filename: it->second) {
            std::string filepath = FileSys.pathByAppendingComponent(sequenceDir.path(), filename);

            // Read file into string 
            std::ifstream in(filepath);
            std::string line;
            std::smatch matches;
            while (std::getline(in, line)) {
                if (std::regex_match(line, matches, _pattern)) {
                    ghoul_assert(matches.size() == 3, "Bad event data formatting. Must have regex 3 matches (source string, start time, stop time).");
                    std::string start = matches[1].str();
                    std::string stop = matches[2].str();

                    TimeRange timeRange;
                    timeRange._min = SpiceManager::ref().ephemerisTimeFromDate(start);
                    timeRange._max = SpiceManager::ref().ephemerisTimeFromDate(stop);

                    _instrumentTimes.push_back({ instrumentID, timeRange });
                    _targetTimes.push_back({ timeRange._min, _target });
                    _captureProgression.push_back(timeRange._min);

                    Image image;
                    image.startTime = timeRange._min;
                    image.stopTime = timeRange._max;
                    image.path = filepath;
                    image.activeInstruments.push_back(instrumentID);
                    image.target = _target;
                    image.projected = false;

                    _subsetMap[_target]._subset.push_back(image);
                    _subsetMap[_target]._range.setRange(image.startTime);
                }
            }
        }
    }
    
    
  
    
    std::stable_sort(_captureProgression.begin(), _captureProgression.end());
    //std::stable_sort(_instrumentTimes.begin(), _instrumentTimes.end());
    std::stable_sort(_targetTimes.begin(), _targetTimes.end(), targetComparer);

    sendPlaybookInformation(PlaybookIdentifierName);
    return true;
}


}