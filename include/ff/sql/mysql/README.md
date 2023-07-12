
Mapping SQL types to C++ types
================

##### Numeric Data Types
| SQL Type of Received Value | C++ Type |
| -------------------------- | ---------|
| BIT(M) | `std::bitset<M>`, M is std::size_t |
| BOOL | TODO |
| TINYINT | `int8_t` |
| TINYINT UNSIGNED| `uint8_t` |
| SMALLINT | `int16_t` |
| SMALLINT UNSIGNED| `uint16_t` |
| MEDIUMINT | `ff::mysql::medium_int` |
| MEDIUMINT UNSINGED | `ff::mysql::medium_uint` |
| INT | `int32_t` |
| INT UNSIGNED| `uint32_t`|
| BIGINT | `int64_t`  |
| BIGINT UNSIGNED | `uint64_t` |
| FLOAT | `float` |
| DOUBLE | `double` |
| DECIMAL(P, D) | `ff::mysql::decimal<P, D>`, 1<=P<=65, 0<=D<=30 |


##### String Data Types
| SQL Type of Received Value | C++ Type |
| -------------------------- | ---------|
| CHAR(M) | `ff::mysql::char_m<M>`, M is uint8_t |
| VARCHAR(M) | `ff::mysql::varchar_m<M>`, M is uint16_t |
| BINARY(M) | `ff::mysql::binary<M>`, M is uint32_t |
| VARBINARY(M) | `ff::mysql::var_binary<M>`, M is uint32_t |
| TEXT | `std::string, or ff::mysql::text`|
| TINYTEXT | `ff::mysql::tiny_text` |
| MEDINUMTEXT | `ff::mysql::medium_text` |
| LONGTEXT | `ff::mysql::long_text`|
| TINYBLOB | `ff::mysql::tiny_blob` |
| BLOB | `ff::mysql::blob` |
| MEDINUMBLOB | `ff::mysql::medium_blob` |
| LONGBLOB | `ff::mysql::long_blob` |
| ENUM(val1, val2, val3)| `char v1[]="val1"; char v2[] = "val2"; char v3[] = "val3"; ff::mysql::enum_m<v1, v2, v3>`|
| SET(val1, val2, val3)| `char v1[]="val1"; char v2[] = "val2"; char v3[] = "val3"; ff::mysql::set_m<v1, v2, v3>`|


##### Date and Time Data Types
| SQL Type of Received Value | C++ Type |
| -------------------------- | ---------|
| DATE | `ff::mysql::date`|
| DATETIME(fsp) | `std::chrono::time_point<std::chrono::system_clock, Duration>`, fsp=0: Duration=std::chrono::seconds; fsp=3: Duration=std::chrono::milliseconds; fsp=6: Duration=std::chrono::microseconds  |
| TIMESTAMP(fsp) | `std::chrono::time_point<std::chrono::steady, Duration>`, fsp=1: Duration=std::chrono::seconds; fsp=3: Duration=std::chrono::milliseconds; fsp=6: Duration=std::chrono::microseconds  |
| TIME(fsp) | fps=0: `std::chrono::seconds`, fps=3: `std::chrono::milliseconds`, fps=6: `std::chrono::microseconds`|
| YEAR | `ff::mysql::year`|


### TODO
1. add arith op for medium_int
2. add boolean type
3. we may have TIMEZONE(UTC, localtime) problem. Need some fix.
4. we may also have range issue for TIMESTAMP. Need some fix.
5. we may use C API instead of cppconn for better performance.
6. add support for GEOMETRY and JSON
7. add SQL function (or operator) support


