#include "calkitsettings.H"
#include <QDataStream>

QDataStream &operator<<(QDataStream &out, const complex<double> &myObj) {
    const double* val = reinterpret_cast<const double*>(&myObj);
    out << val[0];
    out << val[1];
    return out;
}

QDataStream &operator>>(QDataStream &in, complex<double> &myObj) {
    double* val = reinterpret_cast<double*>(&myObj);
    in >> val[0];
    in >> val[1];
    return in;
}


QDataStream &operator<<(QDataStream &out, const string &myObj) {
    int sz = (int)myObj.length();
    out << sz;
    out.writeRawData(myObj.data(), sz);
    return out;
}

QDataStream &operator>>(QDataStream &in, string &myObj) {
    int sz = 0;
    in >> sz;
    myObj.resize(sz);
    if(in.readRawData(&myObj[0], sz) != sz)
        throw runtime_error("short read from QDataStream");
    return in;
}


QDataStream &operator<<(QDataStream &out, const MatrixXcd &myObj) {
    out << (int)myObj.rows();
    out << (int)myObj.cols();
    for(int i=0; i<(myObj.cols()*myObj.rows()); i++)
        out << myObj(i);
    return out;
}

QDataStream &operator>>(QDataStream &in, MatrixXcd &myObj) {
    int rows,cols;
    in >> rows;
    in >> cols;
    if(rows<0 || cols<0 || rows>1024 || cols>1024) {
        fprintf(stderr, "warning: matrix not deserialized because of invalid size %d x %d\n", rows,cols);
        return in;
    }
    myObj.resize(rows,cols);
    for(int i=0; i<(myObj.cols()*myObj.rows()); i++)
        in >> myObj(i);
    return in;
}

template<class S,class T>
QDataStream &operator<<(QDataStream &out, const map<S,T> &m);
template<class S,class T>
QDataStream &operator>>(QDataStream &in, map<S,T> &m);


template<class S,class T>
QDataStream &operator<<(QDataStream &out, const map<S,T> &m) {
    int sz = m.size();
    out << sz;
    for(auto it=m.begin(); it!=m.end(); it++) {
        out << (*it).first;
        out << (*it).second;
    }
    return out;
}
template<class S,class T>
QDataStream &operator>>(QDataStream &in, map<S,T> &m) {
    int sz = 0;
    in >> sz;
    m.clear();
    if(sz<0 || sz > 1024*1024) {
        fprintf(stderr, "warning: map not deserialized because of invalid length %d\n", sz);
        return in;
    }
    for(int i=0;i<sz;i++) {
        S key;
        in >> key;
        in >> m[key];
    }
    return in;
}

QDataStream &operator<<(QDataStream &out, const SParamSeries &myObj) {
    out << myObj.values;
    return out;
}

QDataStream &operator>>(QDataStream &in, SParamSeries &myObj) {
    in >> myObj.values;
    return in;
}


QDataStream &operator<<(QDataStream &out, const CalKitSettings &myObj) {
    out << myObj.calKitModels;
    out << myObj.calKitNames;
    return out;
}

QDataStream &operator>>(QDataStream &in, CalKitSettings &myObj) {
    in >> myObj.calKitModels;
    in >> myObj.calKitNames;
    return in;
}




void serialize(ostream &out, const SParamSeries &obj) {

}

void deserialize(istream &in, SParamSeries &obj) {

}

void serialize(ostream &out, const CalKitSettings &obj) {

}

void deserialize(istream &in, CalKitSettings &obj) {

}
