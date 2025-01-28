
#pragma once 

#include <utils/logger/Logger.h>
#include <utils/system/FileSystem.h>

#include <fstream>
#include <iostream>
#include <vector>

namespace nvd {
namespace cmn {

/// @class CsvWriter provide API to write data in a csv format into a csv file
class CsvWriter
{
public:
    CsvWriter(const std::string& filePath, bool isNew = false)
    {
        LOGINFO("CsvWriter CTor {}", filePath);
        auto op = open_file(filePath, isNew);
        if (op)
            return;

        std::string sPath;
        const size_t lastSlashIdx = filePath.rfind('/');
        if (std::string::npos != lastSlashIdx)
        {
            sPath = filePath.substr(0, lastSlashIdx);
        }
        LOGINFO("createDirectory {}",  sPath);
        auto res = nvd::utils::createDirectory(sPath);
        if (res != nvd::utils::Success) LOGERROR("Failed to create folder {} error code {}", sPath, res);
        open_file(filePath, isNew);
    }

    ~CsvWriter()
    {
        close();
    }

    void close()
    {
        if (m_isOpen)
            m_ofs.close();
        m_isOpen = false;
    }

    bool isOpen() const
    {
        return m_isOpen;
    }

    bool writeHeader(const std::string& sHeader)
    {
        m_ofs << sHeader << "\n";
        return true;
    }

    /// @brief write the vector values into new line
    ///        in the csv file seperated by ',' delimiter
    /// @brief sHeader first column value
    template <typename T>
    bool writeLine(const std::string& sHeader,
                   const std::vector<T>& vec)
    {
        m_ofs << sHeader << ", ";
        return _writeVector(vec);
    }

    /// @brief write the vector values into new line
    ///        in the csv file seperated by ',' delimiter
    template <typename T>
    bool writeLine(const std::vector<T>& vec)
    {
        return _writeVector(vec);
    }

    template <typename T>
    void operator<<(const std::vector<T>& vec)
    {
        writeLine(vec);
    }

private:
    template <typename T>
    bool _writeVector(const std::vector<T>& vec)
    {
        size_t i = 0;
        for (; i < vec.size() - 1; ++i)
        {
            m_ofs << vec[i] << ",";
        }
        // write last value
        m_ofs << vec[i] << "\n";
        return true;
    }

    bool open_file(const std::string& sFilePath, bool isNew)
    {
        std::ios::openmode op = std::ios::out;
        !isNew ? op |= std::ios::app : op;
        m_ofs.open(sFilePath, op);

        if (m_ofs.is_open())
        {
            m_isOpen = true;
            LOGINFO("Open file {} Succeeded", sFilePath);
        }
        return m_isOpen;
    }

private:
    std::ofstream m_ofs;
    bool m_isOpen = false;
};

}}  // namespace

