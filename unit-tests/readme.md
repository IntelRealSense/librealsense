# Unit-Tests

## Running the Unit-Tests

When running **CMake** please make sure the following flag is passed:
`-DBUILD_UNIT_TESTS=true`
After running `make && sudo make install`, you can execute `live-test` to run library unit-tests. 
Make sure you have Intel® RealSense™ device connected. 

## Testing just the Software

If not all unit-tests are passing this can be related to faulty device or problems with the environment setup. 
We support recording test flows at the backend level and running tests on top of mock hardware. This helps us distingwish between hardware problems and software issues. 
You can record unit-test into file using:
`live-test into filename`
To run unit-tests without actual hardware, based on recorded data, run:
`live-test from filename`
This mode of operation lets you test your code on a variety of simulated devices.  

## Controlling Test Execution

We are using [Catch](https://github.com/philsquared/Catch) as our test framework. 