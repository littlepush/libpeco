# PEUtils

Utility tools for PECo

### Include

This project include [`JsonCpp`](https://github.com/open-source-parsers/jsoncpp)

## Install

```
make && make install
```

## Modules

Basically, modules have no dependency, each module can be used individual except `stringutil`, which is almost the basic for all.



### stringutil

`stringutil.h` contains a serious of functions we frequently used in normal project to process string, include the following parts:

* String Trim: Trim white space from a given string

  ```c++
  std::string& left_trim( std::string& s );
  std::string& right_trim(std::string& s);
  std::string& trim(std::string& s);
  ```

* Convert, to lower, to upper or capitalize

  ```c++
    std::string& string_tolower(std::string& s);
    std::string string_tolower(const std::string& s);
    std::string& string_toupper(std::string& s);
    std::string string_toupper(const std::string& s);
    std::string& string_capitalize(std::string& s, std::function<bool(char)> reset_checker);
    std::string& string_capitalize(std::string& s);
    std::string& string_capitalize(std::string& s, const string& reset_carry);
  ```

* Split a given string according to specified carry

  ```c++
  template < typename carry_iterator_t >
  inline std::vector< std::string > split(
  		const string& value, carry_iterator_t b, carry_iterator_t e);
  
  template < typename carry_t >
  inline std::vector< std::string > split(
  		const string& value, const carry_t& carry);
  ```

* Join some components to into a string

  ```c++
  template< typename ComIterator, typename Connector_t >
  inline std::string join(ComIterator begin, ComIterator end, Connector_t c);
  
  template < typename Component_t, typename Connector_t >
  inline std::string join(const Component_t& parts, Connector_t c);
  ```

* String prefix, suffix checking

  ```c++
  bool is_string_start(const std::string& s, const std::string& p);
  std::string check_string_start_and_remove(const std::string& s, const std::string& p);
  
  bool is_string_end(const std::string& s, const std::string& p);
  std::string check_string_end_and_remove(const std::string& s, const std::string& p);
  ```

* Coder

  ```c++
  // URL encode and decode
  std::string url_encode(const std::string& str);
  std::string url_decode(const std::string& str);
  
  // Md5 Sum
  std::string md5(const std::string& str);
  
  // Base64
  std::string base64_encode(const string& str);
  std::string base64_decode(const string& str);
  ```

* Check and Cast

  ```c++
  // Number Check
  // Check if a given string is a number, the string can be a float string
  bool is_number( const std::string& nstr );
  
  // Date Convert
  // Default format is "yyyy-mm-dd hh:mm:ss"
  // This function use `strptime` to parse the string
  time_t dtot( const std::string& dstr );
  time_t dtot( const std::string& dstr, const std::string& fmt );
  ```

  

### gzip

`gzip.h` module is a wrapper of `libz`'s C-API, we use these functions to gzip and gunzip data.

```c++
// Gzip Data
std::string gzip_data( const char* input_str, uint32_t length );
std::string gzip_data( const std::string& input_str );

// GUnzip Data
std::string gunzip_data( const char* input_str, uint32_t length );
std::string gunzip_data( const std::string& input_str );
```



### hex_dump

`hex_dump` is a data formatter object, used to output the data in hex value. It will print the given data in the following format:

```
ADDRESS: HEX HEX HEX...     |original data|
```

Each line contains 16 bytes

#### usage

```c++
std::string _mydata = "any data";
std::cout << pe::co::utils::hex_dump(_mydata) << std::endl;
```

```c++
auto _hd = pe::co::utils::hex_dump(pdata, dlen);
_hd.print(stderr);
```



### sysinfo

`sysinfo` is used to get current host's basic runtime information about CPU and Memory.

`update_sysinfo` should always be the first method to be invoked. It will update current system status and speed up other methods' calucation.

* Get CPU Count

  ```c++
  // Get cpu count
  size_t cpu_count();
  ```

* Get all CPU's load, in percentage

  ```c++
  // Get CPU Load
  std::vector< float > cpu_loads();
  ```

* Get Current Process's CPU usage, in percentage

  ```c++
  // Get self's cpu usage
  float cpu_usage();
  ```

* Get current machine's total memory, in bytes

  ```c++
  // Total Memory
  uint64_t total_memory();
  ```

* Get current system's free memory, in bytes

  ```c++
  // Free Memory
  uint64_t free_memory();
  ```

* Get current process's memory usage, in bytes

  ```c++
  // Memory Usage
  uint64_t memory_usage();
  ```

  

### filemgr

### argparser

### dependency

### eventtrigger

### filternode



## License

MIT License

Copyright (c) 2019 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.