#pragma once
#include<string>
#include<iterator>
#include<fstream>
#include<type_traits>
#include<functional>
#include<algorithm>
#include<QString>
#include<QFile>

class CConfig
{
public:
	static QStringList readPlayList();
	static void writePlayList(const std::vector<QString>& list);

protected:
	static QString m_strPlayListFileName;
};

