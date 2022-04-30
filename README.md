LookupTable  ![GitHub](https://img.shields.io/github/license/zjldemers/LookupTable?label=license)
===

After looking for a decent lookup table implementation for a project I was working on and not finding any that fit my needs, I decided to create my own and share it with the community.

---
> *Note: This implementation is not intended to be the most efficient lookup table in existence, but one that works and has a reasonably intuitive interface to provide users with an option that did not previously exist. It is also hopefully designed such that it is simple to reconfigure to meet an individual project's specific needs.*
---

With that in mind, the LookupTable project implements variations of multi-dimensional lookup tables that utilize linear interpoloation for queries that do not land precisely on a data point.  

The `LookupTableND` class is the base class and most generic implementation that can support tables with two or more dimensions (1-dimension "tables" are left for more trivial data structures).

This repo also includes `LookupTable2D` and `LookupTable3D` classes that are specific implementations for 2-dimensional and 3-dimensional use-cases, respectively.  These are slightly more efficient than the `LookupTableND` with 2 or 3 dimensions as they are able to reduce the number of loops required since their size is known at compile time.




---
## Source Inclusion Guidlines
Add the appropriate header to your project based on your use-case.
1. `LookupTableND.h`: for the N-dimensional Table (will also include `utils.h` internally)
2. `LookupTable2D.h`: for the 2-dimensional Table (will also include `LookupTableND.h` internally)
3. `LookupTable3D.h`: for the 3-dimensional Table (will also include `LookupTableND.h` internally)
4. `LookupTable.h`: for all LookupTable variations



---
## Design
The core underlying data structures to this data structure are a set of `std::vector` objects (yes, I used the STL, sorry to the haters).  The table itself must have at least two dimensions of independent data, along with one `vector` of data corresponding to the dependent data.  You could imagine this as `indep[x][y] = dep[z]`, although it doesn't quite function that way - it's a lookup table, not a matrix, after all.

### Limitations / Restraints
*Data Type*
- One assumption made was that lookup tables of this fashion tend to use floating point values, and frequently need double precision.  With that in mind, rather than templating everything, the decision was made to only support data in the `double` format.
- *Note:* two `typedef`s are used such that `TableData` is a `std::vector<double>` and `TableDataSet` is a `std::vector<TableData>` to provide shorthands.

*Data Format*
- In order to significantly simplify and increase the performance of lookup operations, the data must be formatted such that the independent data values are monotonically increasing (`indep[x][0]` is less than `indep[x][1]`, and so on).

*Data Linearity*
- The interpolation used is simple linear interpolation, and therefore the less linear the provided data is, the less accurate these lookups will be.

*Data Queries*
- All queries of the data must be given within the bounds of the provided data.  That is, extrapolation is not currently considered.  This, in many cases, is the preferred behavior to stop inaccurate results from occuring before they confuse results.  Future work may provide the option to set a flag for extrapolating or not.  For now, attempting to retrieve a data point out of bounds will return an error.


---
## Usage
Most examples will utilize the `LookupTableND` class since it operates as the base class and the other classes have a laregly identical interface.  The key differences will be pointed out.

### *Construction*
The default constructor will leave the table in an "invalid" state, meaning that the data is empty but initialized and ready to be defined.  Custom constructors provide the option of providing table data at construction.  These custom constructors use the `PopulateData` methods internally, which can also be called externally on an existing object.

Note that when populating the table's data, the dimensions must match.  That is, if you have independent data where the first variable has 3 values and the second has 4, the dependent data *must* be of size 12 (3*4=12).

```C++
// Default construction
LookupTableND lut0;
lut0.Valid(); // false

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

// Population outside construction, returns a Boolean representing success
if(lut0.PopulateData(indepData, depData))
    // lut0.Valid() == true <- this would be true in this case
else 
    // lut0.Valid() == false

if(lut1.PopulateData(badData))
    // lut1.Valid() == true
else
    // lut3.Valid() == false <- this would be true in this case
```

The data can also quickly be cleared, if desired, using the `ResetData` method or by overwriting it with a default constructor.  They both result in an "invalid" table that is waiting to be provided with data.


### *Lookup Methods*
These methods, as would be expected, provide the core functionality to the class.  There are two ways to retrieve data from a table, by providing indices or by providing values.

#### LookupByIndices
The `LookupByIndices` methods allow a user to search the table for a precise location based off of indices in the independent data.  This use-case is likely very uncommon as most lookup table uses expect you to provide values that are not explicitly stored in the provided data but are somewhere within its bounds.

In any event, this method is used internally when looking up by value, and therefore it was exposed to the user in case it could be found helpful.  It is notably more efficient than the `LookupByValues` method since there is no need to interpolate, but it is much less flexible due to the same reason.

#### LookupByValue
The alternative is to use the `LookupByValues` method, providing values to find in the independent data as the positions to locate the final dependent value.  This will utilize simple linear interpolation between all of the different points found across all of the dimensions.  The number of interpolation operations is equal to 2<sup>N</sup>-1 where N represents the number of dimensions.  That is, a 2D lookup table will only interpolate 3 times, while a 3D table will interpolate 7 times, a 4D table 15 times, 5D 31 times, and so on... That said, the speed of the lookup relies heavily on the dimensionality of the data provided.

#### Two Main Methods of Use
Due to considerations of both efficiency and preferences of users I have interacted with, each of the LookupTable classes include two different means of performing these lookup operations.  One is to utilize "out variables" (pointers), and the others is to return a custom `utils::Result<T>` object as defined under `utils.h`.

The out variable option is recommended for use-cases where the table is receiving queries especially frequent as it will not require the added overhead of constructing and destructing these (albeit small) `Result` objects.  The difference in performance is likely not large, but if performance is a concern the out variable option is likely to outperform the other ever so slightly.  Future efforts on this project may involve benchmarked comparisons.

```C++
TableDataSet fullData = {
    {1.2, 3.4},
    {5.6, 7.8, 9.0},
    {9.87, 6.54, 3.21, 0.98, 7.65, 4.32}
};

LookupTableND lutND(fullData);
double val;
std::string errMsg;
if(lutND.LookupByIndices({1,2}, &val, &errMsg)) {
    // val contains the value found (4.32 in this example)
}
else { // if an error occurred (not in the example case)...
    // errMsg describes what went wrong (bad values provided, invalid table, etc.)
}
utils::Result<double> res = lutND.LookupByIndices({1,2});
if(res.Valid())
    // res.Value() contains the value found
else
    // res.ErrorMessage() contains the error

// LookupTable2D example rather than LookupTableND where N=2
LookupTable2D lut2D(fullData); // note: using same source data as lutND
if(lut2D.LookupByIndices(1, 2, &val, &errMsg)) // note: no {}'s
    // val contains the value found
else
    // errMsg contains the error

res = lut2D.LookupByIndices(1,2); // note: no {}'s
if(res.Valid())
    // res.Value() contains the value found
else
    // res.ErrorMessage() contains the error

bool valid = lutND.LookupByValues({1.5,8.2}, &val, &errMsg);
res = lut2D.LookupByValues(1.5,8.2);
// Results of this example...
//    valid, res.Valid()          | true
//    val, res.Value()            | 4.33591
//    errMsg, res.ErrorMessage()  | "" (empty string)
```



### *Table Metadata Methods*
Finally, there are a few simple methods for understanding the structure of the LookupTable.
```C++
// To ensure the table is ready for operations...
lutND.Valid();

// To check dimension count (independent data vectors)...
lutND.Dimension();

// To check size of the dependent data...
lutND.DepDataSize();

// To check the size of a single independent data dimension...
size_t aDimension = 0; // 0 to N-1
lutND.IndepDataSize(aDimension);
```



---
## State of Development
As stated above, there are some areas of consideration for future work, but at the moment this project is currently in the state of "good enough".  That said, I am also open to recommendations for improvement and would be happy to implement them if there are users that would benefit from it.  Please feel free to raise issues on [the project's github repo](https://github.com/zjldemers/LookupTable).


---
## License
This project is licensed under the [MIT License](https://mit-license.org/).