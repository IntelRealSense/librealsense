#include "fw-log-data.h"
#include <sstream>
#include <iomanip>
#include <locale>
#include <string>


# define SET_WIDTH_AND_FILL(num, element) \
std::setfill(' ')<< std::setw(num)<<std::left<<element \


FWlogData::FWlogData(void)
{
	magicNumber = 0;
	severity = 0;
	fileId = 0;
	groupId = 0;
	eventId = 0;
	line = 0;
	sequence = 0;
	p1 = 0;
	p2 = 0;
	p3 = 0;
	timeStamp = 0;
	delta = 0;

	message = "";
	fileName = "";
}


FWlogData::~FWlogData(void)
{
}


std::string FWlogData::ToString()
{
	std::stringstream fmt;


	fmt << SET_WIDTH_AND_FILL(6, sequence)
		<< SET_WIDTH_AND_FILL(30, fileName)
		<< SET_WIDTH_AND_FILL(6, groupId)
		<< SET_WIDTH_AND_FILL(15, threadName)
		<< SET_WIDTH_AND_FILL(6, severity)
		<< SET_WIDTH_AND_FILL(6, line)
		<< SET_WIDTH_AND_FILL(15, timeStamp)
		<< SET_WIDTH_AND_FILL(15, delta)
		<< SET_WIDTH_AND_FILL(30, message);

	auto str = fmt.str();
	return str;
}
