LookupTable  ![GitHub](https://img.shields.io/github/license/zjldemers/LookupTable?label=license)
===

After searching for a decent lookup table implementation for a project I was working on and not finding any that fit my needs, I decided to create my own and share it with the community.

---
> *Note: This implementation is not intended to be the most efficient lookup table in existence, but one that works and has a reasonably intuitive interface to provide users with an option that did not previously exist. It is also hopefully designed such that it is simple to reconfigure to meet an individual project's specific needs.*
---

With that in mind, the LookupTable project implements variations of multi-dimensional lookup tables that utilize linear interpoloation for queries that do not land precisely on a data point.  

The `LookupTableND` class is the base class and most generic implementation that can support tables with two or more dimensions (1-dimension "tables" are left for more trivial data structures).

This repo also includes `LookupTable2D` and `LookupTable3D` classes that are specific implementations for 2-dimensional and 3-dimensional use-cases, respectively.  These are slightly more efficient than the `LookupTableND` with N set to 2 or 3 as they are able to reduce the number of loops required since their size is known at compile time.




---
## Source Inclusion Guidlines
Add the appropriate header to your project based on your use-case.
1. `LookupTableND.h`: for the N-dimensional Table (will also include `LookupUtils.h` internally)
2. `LookupTable2D.h`: for the 2-dimensional Table (will also include `LookupTableND.h` internally)
3. `LookupTable3D.h`: for the 3-dimensional Table (will also include `LookupTableND.h` internally)
4. `LookupTable.h`: for all LookupTable variations



---
## Design
The core underlying data structures used within these tables are a set of `std::vector` objects (yes, I used the STL; sorry to the haters).  The table itself must have at least two dimensions of independent data, along with one `vector` of data corresponding to the dependent data.  You could imagine this functioning along the lines of `indep[x][y] = dep[z]`.

### Limitations / Restraints
*Data Type*
- One assumption made was that lookup tables of this fashion tend to use floating point values and frequently need double precision.  With that in mind, rather than templating everything, the decision was made to only support data in the `double` format.
- *Note:* two `typedef`s are used such that `TableData` is a `std::vector<double>` and `TableDataSet` is a `std::vector<TableData>` to provide shorthands.

*Data Format*
- In order to significantly simplify and increase the performance of lookup operations, the data must be formatted such that the independent data values are monotonically increasing (`indep[x][0]` is less than `indep[x][1]`, and so on).

*Data Linearity*
- The interpolation used is simple linear interpolation, assuming that the data can be assumed linear in between the provided data points (if small deltas or linear behavior).  Therefore, the more space between data points in the provided data, the less accurate these lookups will be - as is the case for all lookup implementations.

*Data Bounds*
- All queries of the data must be given within the bounds of the provided data.  That is, extrapolation is not currently implemented.  This, in many cases, is the preferred behavior to stop inaccurate results from occuring before they confuse results.  Future work may provide the option to set a flag for extrapolating or not.  For now, attempting to retrieve a data point out of bounds will throw an exception or return an error, depending on the lookup method used (see usage of lookup methods below).


---
## Usage
Most examples in this README will utilize the `LookupTableND` class since it operates as the base class and the other classes have a laregly identical interface.  The key differences will be pointed out.

### *Construction*
The default constructor will leave the table in an "invalid" state, meaning that the data is empty but initialized and ready to be defined.  Custom constructors provide the option of providing table data at construction.  These custom constructors use the `PopulateData` methods internally, which can also be called externally on an existing object.

Note that when populating the table's data, the dimensions must match.  That is, if you have independent data where the first variable has 3 values and the second has 4, the dependent data *must* be of size 12 (3*4=12).

```C++
// Default construction
LookupTableND lut0;
lut0.Valid(); // false after default construction and no data input yet

// Custom construction - separate data sets
TableDataSet indepData = {
    {1.2, 3.4},
    {5.6, 7.8, 9.0}
};
TableData depData = {9.87, 6.54, 3.21, 0.98, 7.65, 4.32};
LookupTableND lut1(indepData, depData);
lut1.Valid(); // true

// Custom construction - single data set
TableDataSet fullData = {
    {1.2, 3.4},                          // indep[0]
    {5.6, 7.8, 9.0},                     // indep[1]
    {9.87, 6.54, 3.21, 0.98, 7.65, 4.32} // dep
    // Note: this is a 2-dimensional table (indep data defines dimensionality)
};
LookupTableND lut2(fullData);
lut2.Valid(); // true

// Invalid construction
TableDataSet badData = {
    {0.1, 0.3, 0.2}, // invalid: not monotonically increasing
    {0.4, 0.5},
    {0.6, 0.7, 0.8}  // invalid: bad dimensions (should be size=6)
}
LookupTableND lut3(badData);
lut3.Valid(); // false

// Population outside construction returns a Boolean representing success
if(lut0.PopulateData(indepData, depData))
    // lut0.Valid() == true <- example would lead to this
else 
    // lut0.Valid() == false

if(lut1.PopulateData(badData))
    // lut1.Valid() == true
else
    // lut3.Valid() == false <- example would lead to this
```

The data can also quickly be cleared, if desired, using the `ResetData()` method or by overwriting it with a default constructor.  They both result in an "invalid" table that is waiting to be provided with data.


### *Lookup Methods*
These methods, as would be expected, provide the core functionality to the class.  Table dependent data can be retrieved by index (direct access lookups) or by value (using linear interpolation between points).  For the sake of maximum flexibility, each of those methods have three formats of managing errors along the way: exceptions, out variables, and a custom class return type.  These explained in more depth later.

#### LookupByIndices
The `LookupByIndices` methods allow a user to search the table for a precise location based off of indices in the independent data.  This use-case is likely very uncommon as most lookup table uses expect you to provide values that are not explicitly stored in the provided data but are somewhere within its bounds.

In any event, this method is used internally when looking up by value, and therefore it was exposed to the user in case it could be found helpful.  It is notably more efficient than the `LookupByValues` method since there is no need to interpolate, but it is much less flexible due to the same reason.

#### LookupByValues
The alternative is to use the `LookupByValues` methods, providing values to find in the independent data as the positions to locate the final dependent value.  This will utilize simple linear interpolation between all of the different points found across all of the dimensions.  The number of interpolation operations is equal to 2<sup>N</sup>-1 where N represents the number of dimensions.  That is, a 2D lookup table will only interpolate 3 times, while a 3D table will interpolate 7 times, a 4D table 15 times, 5D 31 times, and so on... That said, the speed of the lookup relies heavily on the dimensionality of the data provided.

#### Three Main Schemas
Due to considerations of standards, efficiency, and preferences of users I have interacted with, each of the LookupTable classes include three different means of performing these lookup operations.  The main schema, used by those with the `Lookup`- prefix, accesses the data assuming all inputs are valid and will throw exceptions if errors occur (following C++ standards).  Aside from that, however, two others are also provided as a sort of soft lookup where one utilizes "out variables" (pointers), and the other returns a custom `utils::Result<T>` object as defined under `LookupUtils.h`.  These two "soft" lookups contain the `Query`- prefix instead of the `Lookup`- prefix.

When to use which schema is entirely up to the user, but here are some notes to consider.
- The `Lookup`- methods will throw exceptions of types `std::exception` (invalid table, empty data vector) and `std::invalid_argument` (index/value out of bounds, too many inputs).
- The `Query`- methods use the `Lookup`- methods under the hood and simply catch the exception internally, reporting an error.  Therefore, there is no difference in the result found during the lookup.
- While no benchmarks have been setup to confirm this yet, the expected order of speed (from fastest to slowest) is (1) standard `Lookup`, (2) query with out variables, and (3) query with `Result`.  In all likelihood, their performance is extremely similar, but the slight overhead of creating try/catch blocks or constructing/destructing the `Result<T>` object will presumably slow those methods down slightly.

#### Standard Lookup
```C++
double value;
try {
    // ND Table
    value = lutND.LookupByIndices({i0, ..., iN-1});
    value = lutND.LookupByValues({v0, ..., vN-1});
    // throws exception if input count does not match dimension count

    // 2D Table
    value = lut2D.LookupByIndices(i0, i1);
    value = lut2D.LookupByValues(v0, v1);
    
    // 3D Table
    value = lut3D.LookupByIndices(i0, i1, i2);
    value = lut3D.LookupByValues(v0, v1, v2);
} catch(std::exception& e) {
    // do something with exception
}
```

#### Query: Out Variables
```C++
double value;
std::string errMsg;

if(lutND.QueryByIndices({i0,i1}, &value, &errMsg)) // ND table with N=2
    // do something with *value
else
    // do something with *errMsg

if(lut2D.QueryByValues(v0, v1, &value, &errMsg))
    // do something with *value
else
    // do something with *errMsg
```

#### Query: Result
```C++
utils::Result<double> res;

res = lut3D.QueryByIndices(i0, i1, i2);
if(res.Valid())
    // do something with res.Value()
else
    // do something with res.ErrorMessage()
res = lutND.QueryByValues({v0,v1,v2}); // ND table with N=3
if(res.Valid())
    // do something with res.Value()
else
    // do something with res.ErrorMessage()
```

As seen above, the usage schemas for lookups are very flexible.  Hopefully one or more of these will be found useful in your project.



### *Table Metadata Methods*
Finally, there are a few simple methods for understanding the structure of the LookupTable.
```C++
// To ensure the table is ready for operations...
lutND.Valid();

// To check dimension count (number of independent data vectors)...
lutND.Dimensions();

// To check size of the dependent data...
lutND.DepDataSize();

// To check the size of a single independent data dimension...
lutND.IndepDataSize(aDimension); // where aDimension is in range [0, N-1]
```



---
## State of Development
As stated above, there are some areas of consideration for future work, but at the moment this project is currently in the state of "good enough".  That said, I am also open to recommendations for improvement and would be happy to implement them if there are users that would benefit from it.  Please feel free to raise issues on [the project's github repo](https://github.com/zjldemers/LookupTable).


---
## License
This project is licensed under the [MIT License](https://mit-license.org/).