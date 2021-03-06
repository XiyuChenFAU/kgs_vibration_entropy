/*

Excited States software: KGS
Contributors: See CONTRIBUTORS.txt
Contact: kgs-contact@simtk.org

Copyright (C) 2009-2017 Stanford University

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

This entire text, including the above copyright notice and this permission notice
shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

*/

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

#define CTK_PI 3.14159265


typedef std::pair<std::string,std::string> CovBond;

class Util {
 public:
  static std::string d2s (double x);
  static std::string f2s (float x);
  static std::string i2s (int x);
  static std::string i2s(int x, int length);
  static bool stob (std::string s);
  static std::string trim (std::string s,char c=' ');
  static std::string getPath (std::string s);
  static std::string getBaseName (std::string s);
  static std::string formatNumber(std::string number, int digits_num);
  static float round(float number, int precision);
  static double round(double number, int precision);
  static std::string cutDecimal(float number, int decimals);

  static std::vector<int>& split( const std::string &s, char delim, std::vector<int>& numbers );
  static std::vector<double>& split( const std::string &s, char delim, std::vector<double>& numbers );
  static std::vector<std::string>& split( const std::string &s, char delim, std::vector<std::string> &words );
  static std::vector<std::string> split( const std::string &s, char delim );
  static std::vector<std::string> split( const std::string &s, const std::string& delim );
  static bool contains( const std::string& s, const std::string& substring );
  static bool startsWith( const std::string& s, const std::string& substring );

};

#endif
