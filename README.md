# Chrolis++
Custom CSV-based protocol for the ThorLabs Chrolis 6-wavelength LED source.
## CSV format
The CSV file should have 5 columns. Each row defines a sequence of light pulses of the same LED, and these sequences are executed one after the other. The rows in order:
* The LED index (0-5)
* Length of each individual light pulse (integer, ms; stored as (signed) long)
* Time between pulses (integer, ms, (signed) long)
* Number of pulses (integer, (signed) long)
* Brightness (integer, 0-1000, where 1000 is 100.0%; e.g. 123 is 12.3%, same control as in the Chrolis application)

  
