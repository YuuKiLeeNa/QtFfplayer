#include "CConfig.h"


QStringList CConfig::readPlayList()
{
    QStringList sets;
    QFile file(m_strPlayListFileName);
    bool isOk = file.open(QIODevice::ReadOnly);
    if (isOk)
    {
        QString str = file.readAll();
        sets = str.split('\n');
        sets.erase(std::remove(sets.begin(), sets.end(), QString()), sets.end());
       /* while (file.atEnd() == false)
        {
            sets.push_back(file.readLine());
        }*/
    }
    file.close();
    return sets;
}

void CConfig::writePlayList(const std::vector<QString>&list) 
{
    QFile file;
    file.setFileName(m_strPlayListFileName);
    bool isOk = file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!list.empty())
    {
        QString str;
        for (auto& ele : list)
            str += ele + "\n";
        file.write(str.toUtf8());
    }
    file.close();
}

//template<typename ITER>
//void CConfig::writePlayList(ITER beg, ITER end)
//{
//	std::ostream_iterator<std::wstring, wchar_t>iter(std::wofstream(m_strPlayListFileName));
//	std::copy(beg, end, [&iter](typename std::iterator_traits<ITER>::value_type&& v)
//		{
//			*iter++ = std::forward<decltype(v)>(v);
//			*iter++ = L'\n';
//		});
//}
//
//template<typename CONTAIN>
//void CConfig::writePlayList(CONTAIN&& t) 
//{
//	writePlayList(std::make_move_iterator(t.begin()), std::make_move_iterator(t.end()));
//}

QString CConfig::m_strPlayListFileName = "playList";