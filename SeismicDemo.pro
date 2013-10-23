#-------------------------------------------------
#
# Project created by QtCreator 2013-10-19T11:01:06
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SeismicDemo
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    utility.cpp \
    geolib/svd_linsolve.cc \
    geolib/svd_decomposition.cc \
    geolib/methods_sampleInterpolation.cc \
    geolib/methods_rotation.cc \
    geolib/methods_pzsum.cc \
    geolib/methods_polarity_correction.cc \
    geolib/methods_orientation.cc \
    geolib/methods_number_conversions.cc \
    geolib/methods_linefit.cc \
    geolib/methods_ccp.cc \
    geolib/geolib_string_utils.cc \
    geolib/geolib_mem.cc \
    geolib/geolib_math.cc \
    geolib/geolib_endian.cc \
    geolib/fft.cc \
    geolib/csToken.cc \
    geolib/csTimeStretch.cc \
    geolib/csTimer.cc \
    geolib/csTableNew.cc \
    geolib/csTableAll.cc \
    geolib/csTable.cc \
    geolib/csStandardHeaders.cc \
    geolib/csSortManager.cc \
    geolib/csSelectionFieldInt.cc \
    geolib/csSelectionFieldDouble.cc \
    geolib/csSelection.cc \
    geolib/csRotation.cc \
    geolib/csNMOCorrection.cc \
    geolib/csIReader.cc \
    geolib/csIOSelection.cc \
    geolib/csInterpolation.cc \
    geolib/csHeaderInfo.cc \
    geolib/csGeolibUtils.cc \
    geolib/csFlexNumber.cc \
    geolib/csFlexHeader.cc \
    geolib/csFileUtils.cc \
    geolib/csFFTTools.cc \
    geolib/csFFTDesignature.cc \
    geolib/csException.cc \
    geolib/csEquationSolver.cc \
    geolib/cseis_curveFitting.cc \
    geolib/csDespike.cc \
    geolib/csAbsoluteTime.cc \
    io/csSeismicWriter_ver.cc \
    io/csSeismicReader_ver03.cc \
    io/csSeismicReader_ver02.cc \
    io/csSeismicReader_ver01.cc \
    io/csSeismicReader_ver00.cc \
    io/csSeismicReader_ver.cc \
    io/csSeismicIOConfig.cc \
    io/csRSFWriter.cc \
    io/csRSFReader.cc \
    io/csRSFHeader.cc \
    io/csGeneralSeismicReader.cc \
    io/csASCIIFileReader.cc \
    segd/csStandardSegdHeader.cc \
    segd/csSegdReader.cc \
    segd/csSegdHeader_SEAL.cc \
    segd/csSegdHeader_GEORES.cc \
    segd/csSegdHeader_DIGISTREAMER.cc \
    segd/csSegdHeader.cc \
    segd/csSegdHdrValues.cc \
    segd/csSegdFunctions.cc \
    segd/csSegdBuffer.cc \
    segd/csNavInterface.cc \
    segd/csNavHeader.cc \
    segd/csGCS90Header.cc \
    segd/csExternalHeader.cc \
    system/csUserParam.cc \
    system/csUserConstant.cc \
    system/csTracePool.cc \
    system/csTraceHeaderInfoPool.cc \
    system/csTraceHeaderDef.cc \
    system/csTraceHeaderData.cc \
    system/csTraceHeader.cc \
    system/csTraceGather.cc \
    system/csTraceData.cc \
    system/csTrace.cc \
    system/csTableManagerNew.cc \
    system/csTableManager.cc \
    system/csSuperHeader.cc \
    system/csStackUtil.cc \
    system/csSelectionManager.cc \
    system/csSeismicWriter.cc \
    system/csSeismicReader.cc \
    system/csParamManager.cc \
    system/csParamDef.cc \
    system/csMemoryPoolManager.cc \
    system/csLogWriter.cc \
    system/csInitExecEnv.cc \
    system/csExecPhaseDef.cc \
    system/cseis_methods.cc \
    system/cseis_help.cc

HEADERS  += mainwindow.h \
    model.h \
    utility.h \
    geolib/TableValueList.h \
    geolib/methods_number_conversions.h \
    geolib/geolib_string_utils.h \
    geolib/geolib_platform_dependent.h \
    geolib/geolib_methods.h \
    geolib/geolib_mem.h \
    geolib/geolib_math.h \
    geolib/geolib_endian.h \
    geolib/geolib_defines.h \
    geolib/csVector.h \
    geolib/csToken.h \
    geolib/csTimeStretch.h \
    geolib/csTimer.h \
    geolib/csTimeFunction.h \
    geolib/csTime.h \
    geolib/csTableValueList.h \
    geolib/csTableNew.h \
    geolib/csTableAll.h \
    geolib/csTable.h \
    geolib/csStandardHeaders.h \
    geolib/csStack.h \
    geolib/csSortManager.h \
    geolib/csSortedVector.h \
    geolib/csSort.h \
    geolib/csSelectionFieldInt.h \
    geolib/csSelectionFieldDouble.h \
    geolib/csSelectionField.h \
    geolib/csSelection.h \
    geolib/csRotation.h \
    geolib/csQueue.h \
    geolib/csNMOCorrection.h \
    geolib/csMatrixFStyle.h \
    geolib/csMathFunction.h \
    geolib/csMathConstant.h \
    geolib/csKey.h \
    geolib/csIReader.h \
    geolib/csIOSelection.h \
    geolib/csInterpolation.h \
    geolib/csHeaderInfo.h \
    geolib/csGeolibUtils.h \
    geolib/csFlexNumber.h \
    geolib/csFlexHeader.h \
    geolib/csFileUtils.h \
    geolib/csFFTTools.h \
    geolib/csFFTDesignature.h \
    geolib/csException.h \
    geolib/csEquationSolver.h \
    geolib/cseis_curveFitting.h \
    geolib/csDespike.h \
    geolib/csCompareVector.h \
    geolib/csCollection.h \
    geolib/csByteConversions.h \
    geolib/csBitStorage.h \
    geolib/csAbsoluteTime.h \
    io/csSeismicWriter_ver00.h \
    io/csSeismicWriter_ver.h \
    io/csSeismicWriterConfig_ver.h \
    io/csSeismicReader_ver03.h \
    io/csSeismicReader_ver02.h \
    io/csSeismicReader_ver01.h \
    io/csSeismicReader_ver00.h \
    io/csSeismicReader_ver.h \
    io/csSeismicIOConfig.h \
    io/csRSFWriter.h \
    io/csRSFReader.h \
    io/csRSFHeader.h \
    io/csIODefines.h \
    io/csGeneralSeismicReader.h \
    io/csASCIIFileReader.h \
    segd/csStandardSegdHeader.h \
    segd/csSegdReader.h \
    segd/csSegdHeader_SEAL.h \
    segd/csSegdHeader_GEORES.h \
    segd/csSegdHeader_DIGISTREAMER.h \
    segd/csSegdHeader.h \
    segd/csSegdHdrValues.h \
    segd/csSegdFunctions.h \
    segd/csSegdDefines.h \
    segd/csSegdBuffer.h \
    segd/csNavInterface.h \
    segd/csNavHeader.h \
    segd/csGCS90Header.h \
    segd/csExternalHeader.h \
    system/csUserParam.h \
    system/csUserConstant.h \
    system/csTracePool.h \
    system/csTraceHeaderInfoPool.h \
    system/csTraceHeaderInfo.h \
    system/csTraceHeaderDef.h \
    system/csTraceHeaderData.h \
    system/csTraceHeader.h \
    system/csTraceGather.h \
    system/csTraceData.h \
    system/csTrace.h \
    system/csTableManagerNew.h \
    system/csTableManager.h \
    system/csSuperHeader.h \
    system/csStackUtil.h \
    system/csSelectionManager.h \
    system/csSeismicWriter.h \
    system/csSeismicReader.h \
    system/csRunManager.h \
    system/csParamManager.h \
    system/csParamDescription.h \
    system/csParamDef.h \
    system/csModuleIndexStack.h \
    system/csModule.h \
    system/csMethodRetriever.h \
    system/csMemoryPoolManager.h \
    system/csLogWriter.h \
    system/csInitExecEnv.h \
    system/csHelp.h \
    system/csExecPhaseDef.h \
    system/cseis_module_defines.h \
    system/cseis_includes.h \
    system/cseis_defines.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -std=gnu++11
