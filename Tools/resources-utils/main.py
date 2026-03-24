from pathlib import Path
from array import array
import numpy as np

dataTypes = ["dataAmp", "dataFreq", "datadureTabs", "dataVectorInterval"]

MAX_PARTIELS = 200
NOMBRE_DE_NOTES = 80
MAX_TAB = 1000

"""
from Sirene.h :
#define NOMBRE_DE_NOTE 80
#define MAX_TAB 1000
#define MAX_Partiel 200
...
float tabAmp[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
float tabFreq[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
float dureTabs[NOMBRE_DE_NOTE][3]; // 0 = dureTab en samples // 1 = nombreMax de Tab // 2 = FreqMoyenne
float vectorInterval[392]; // ???
"""

tabAmpSize = NOMBRE_DE_NOTES * MAX_TAB * MAX_PARTIELS
tabFreqSize = tabAmpSize
dureTabsSize = NOMBRE_DE_NOTES * 3
vectorIntervalSize = 392

def make_path_for(dataType, sirenIndex):
    if dataType not in dataTypes:
        dataType = dataTypes[0]
    sirenIndex = min(7, max(1, sirenIndex))
    if (sirenIndex == 2 and not dataType is "dataVectorInterval") or sirenIndex == 6:
        sirenIndex = sirenIndex - 1

    # Build a stable path relative to this script (not the current working directory)
    script_dir = Path(__file__).resolve().parent
    resource_path = (script_dir / f"../../Resources/{dataType}S{sirenIndex}").resolve()
    return resource_path

def get_float_array_from_raw_data_file_path(path):
    # Read as binary to avoid UTF-8 decoding errors on non-text files
    with path.open("rb") as f:
        data = f.read()
        fdata = array('f', data) # this casts raw bytes to an array of float32's

    return fdata

def process_amp_data(amp_data):
    fdata = np.resize(np.array(amp_data), (NOMBRE_DE_NOTES, MAX_TAB, MAX_PARTIELS))
    return fdata # amp_data[0]

def process_freq_data(freq_data):
    fdata = np.resize(np.array(freq_data), (NOMBRE_DE_NOTES, MAX_TAB, MAX_PARTIELS))
    return fdata # freq_data[0]

def process_duree_data(duree_data):
    fdata = np.resize(np.array(duree_data), (NOMBRE_DE_NOTES, 3))
    return fdata # duree_data[0]

def process_vector_interval_data(vector_interval_data):
    fdata = np.resize(np.array(vector_interval_data), (392))
    return fdata # vector_interval_data[0]

"""
Resources utilities module for handling data files.

This module provides functions to manage and process data files from a specific directory structure.
It includes functions for creating file paths, reading binary data, and processing different types of data files.
"""

# see Siren.cpp : 24 and 36 note offsets seem to correspond to zero-padded
# beginning of datadureTabs files

def main():
    print("Hello from resources-utils!")

    for i in range(1, 2):
        i = 1
        amp_path = make_path_for("dataAmp", i)
        freq_path = make_path_for("dataFreq", i)
        duree_path = make_path_for("datadureTabs", i)
        vector_interval_path = make_path_for("dataVectorInterval", i)

        amp_data = get_float_array_from_raw_data_file_path(amp_path)
        freq_data = get_float_array_from_raw_data_file_path(freq_path)
        duree_data = get_float_array_from_raw_data_file_path(duree_path)
        vector_interval_data = get_float_array_from_raw_data_file_path(vector_interval_path)

    # resource_path = make_path_for("datadureTabs", 1)
    # Read as binary to avoid UTF-8 decoding errors on non-text files

    fdata = duree_data
    count = 0

    for f in fdata:
        if f != 0.0:
            break
        count = count + 1

    print(f"{count} first concommitent null elements")

        # for d in data:
            # print(f"{(d):02x}", end="")
        # print(len(data))
        # print(len(fdata))

        # tabAmp[NOMBRE_DE_NOTE][MAX_TAB][MAX_Partiel];
        # unit_size = len(fdata) / (NOMBRE_DE_NOTES * MAX_TAB * MAX_PARTIELS)

    theoretical = (NOMBRE_DE_NOTES * MAX_TAB * MAX_PARTIELS)

    print(f"Number of notes: {NOMBRE_DE_NOTES},")
    print(f"max tabs: {MAX_TAB},")
    print(f"max partiels: {MAX_PARTIELS}")
    print(f"Theoretical : {theoretical}, Actual : {(len(fdata) - fdata.count(0.0)) / 3}")

    print(f"{len(fdata) - fdata.count(0.0)} non null elements in a {len(fdata)} sized array")
    # print(f"Read {len(fdata)} floats from: {resource_path}")
    # print(f"Unit size {unit_size}")


if __name__ == "__main__":
    main()
