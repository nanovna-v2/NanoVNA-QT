#ifndef CALKITSETTINGS_H
#define CALKITSETTINGS_H
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <xavna/common.H>
#include "touchstone.H"
using namespace std;
using namespace xaxaxa;

// the in-memory structure that holds the calibration kit settings
struct CalKitSettings {
    // if any given cal kit type is not present here, it should be assumed
    // to use ideal parameters
    map<string, SParamSeries> calKitModels;
    map<string, string> calKitNames;
};
#ifdef Q_DECLARE_METATYPE
Q_DECLARE_METATYPE(CalKitSettings);
#endif

class QDataStream;

// binary serializers for QSettings

QDataStream &operator<<(QDataStream &out, const complex<double> &myObj);
QDataStream &operator>>(QDataStream &in, complex<double> &myObj);

QDataStream &operator<<(QDataStream &out, const string &myObj);
QDataStream &operator>>(QDataStream &in, string &myObj);

QDataStream &operator<<(QDataStream &out, const MatrixXcd &myObj);
QDataStream &operator>>(QDataStream &in, MatrixXcd &myObj);

QDataStream &operator<<(QDataStream &out, const SParamSeries &myObj);
QDataStream &operator>>(QDataStream &in, SParamSeries &myObj);

QDataStream &operator<<(QDataStream &out, const CalKitSettings &myObj);
QDataStream &operator>>(QDataStream &in, CalKitSettings &myObj);

// text serializers
void serialize(ostream& out, const SParamSeries& obj);
void deserialize(istream& in, SParamSeries& obj);

void serialize(ostream& out, const CalKitSettings& obj);
void deserialize(istream &in, CalKitSettings &obj);

#endif // CALKITSETTINGS_H
