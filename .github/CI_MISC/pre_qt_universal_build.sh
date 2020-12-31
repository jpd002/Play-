#!/bin/bash

qt_base=$1
qt_base2=$2

pushd $qt_base

cp -R lib/ lib_x64/
cp -R plugins/ plugins_x64/

lipo -create -output lib/QtMultimediaQuick.framework/Versions/5/QtMultimediaQuick lib_x64/QtMultimediaQuick.framework/Versions/5/QtMultimediaQuick $qt_base2/lib/QtMultimediaQuick.framework/Versions/5/QtMultimediaQuick
lipo -create -output lib/QtQuickControls2.framework/Versions/5/QtQuickControls2 lib_x64/QtQuickControls2.framework/Versions/5/QtQuickControls2 $qt_base2/lib/QtQuickControls2.framework/Versions/5/QtQuickControls2
lipo -create -output lib/QtQuickParticles.framework/Versions/5/QtQuickParticles lib_x64/QtQuickParticles.framework/Versions/5/QtQuickParticles $qt_base2/lib/QtQuickParticles.framework/Versions/5/QtQuickParticles
lipo -create -output lib/QtRemoteObjects.framework/Versions/5/QtRemoteObjects lib_x64/QtRemoteObjects.framework/Versions/5/QtRemoteObjects $qt_base2/lib/QtRemoteObjects.framework/Versions/5/QtRemoteObjects
lipo -create -output lib/Qt3DInput.framework/Versions/5/Qt3DInput lib_x64/Qt3DInput.framework/Versions/5/Qt3DInput $qt_base2/lib/Qt3DInput.framework/Versions/5/Qt3DInput
lipo -create -output lib/QtScriptTools.framework/Versions/5/QtScriptTools lib_x64/QtScriptTools.framework/Versions/5/QtScriptTools $qt_base2/lib/QtScriptTools.framework/Versions/5/QtScriptTools
lipo -create -output lib/QtNetworkAuth.framework/Versions/5/QtNetworkAuth lib_x64/QtNetworkAuth.framework/Versions/5/QtNetworkAuth $qt_base2/lib/QtNetworkAuth.framework/Versions/5/QtNetworkAuth
lipo -create -output lib/QtDataVisualization.framework/Versions/5/QtDataVisualization lib_x64/QtDataVisualization.framework/Versions/5/QtDataVisualization $qt_base2/lib/QtDataVisualization.framework/Versions/5/QtDataVisualization
lipo -create -output lib/Qt3DQuickRender.framework/Versions/5/Qt3DQuickRender lib_x64/Qt3DQuickRender.framework/Versions/5/Qt3DQuickRender $qt_base2/lib/Qt3DQuickRender.framework/Versions/5/Qt3DQuickRender
lipo -create -output lib/Qt3DQuickExtras.framework/Versions/5/Qt3DQuickExtras lib_x64/Qt3DQuickExtras.framework/Versions/5/Qt3DQuickExtras $qt_base2/lib/Qt3DQuickExtras.framework/Versions/5/Qt3DQuickExtras
lipo -create -output lib/QtQuick3DRender.framework/Versions/5/QtQuick3DRender lib_x64/QtQuick3DRender.framework/Versions/5/QtQuick3DRender $qt_base2/lib/QtQuick3DRender.framework/Versions/5/QtQuick3DRender
lipo -create -output lib/QtDesigner.framework/Versions/5/QtDesigner lib_x64/QtDesigner.framework/Versions/5/QtDesigner $qt_base2/lib/QtDesigner.framework/Versions/5/QtDesigner
lipo -create -output lib/QtNfc.framework/Versions/5/QtNfc lib_x64/QtNfc.framework/Versions/5/QtNfc $qt_base2/lib/QtNfc.framework/Versions/5/QtNfc
lipo -create -output lib/QtQuick3DAssetImport.framework/Versions/5/QtQuick3DAssetImport lib_x64/QtQuick3DAssetImport.framework/Versions/5/QtQuick3DAssetImport $qt_base2/lib/QtQuick3DAssetImport.framework/Versions/5/QtQuick3DAssetImport
lipo -create -output lib/QtBodymovin.framework/Versions/5/QtBodymovin lib_x64/QtBodymovin.framework/Versions/5/QtBodymovin $qt_base2/lib/QtBodymovin.framework/Versions/5/QtBodymovin
lipo -create -output lib/QtQuickWidgets.framework/Versions/5/QtQuickWidgets lib_x64/QtQuickWidgets.framework/Versions/5/QtQuickWidgets $qt_base2/lib/QtQuickWidgets.framework/Versions/5/QtQuickWidgets
lipo -create -output lib/Qt3DQuickInput.framework/Versions/5/Qt3DQuickInput lib_x64/Qt3DQuickInput.framework/Versions/5/Qt3DQuickInput $qt_base2/lib/Qt3DQuickInput.framework/Versions/5/Qt3DQuickInput
lipo -create -output lib/Qt3DQuickScene2D.framework/Versions/5/Qt3DQuickScene2D lib_x64/Qt3DQuickScene2D.framework/Versions/5/Qt3DQuickScene2D $qt_base2/lib/Qt3DQuickScene2D.framework/Versions/5/Qt3DQuickScene2D
lipo -create -output lib/Qt3DRender.framework/Versions/5/Qt3DRender lib_x64/Qt3DRender.framework/Versions/5/Qt3DRender $qt_base2/lib/Qt3DRender.framework/Versions/5/Qt3DRender
lipo -create -output lib/QtQuick3DRuntimeRender.framework/Versions/5/QtQuick3DRuntimeRender lib_x64/QtQuick3DRuntimeRender.framework/Versions/5/QtQuick3DRuntimeRender $qt_base2/lib/QtQuick3DRuntimeRender.framework/Versions/5/QtQuick3DRuntimeRender
lipo -create -output lib/QtHelp.framework/Versions/5/QtHelp lib_x64/QtHelp.framework/Versions/5/QtHelp $qt_base2/lib/QtHelp.framework/Versions/5/QtHelp
lipo -create -output lib/QtPrintSupport.framework/Versions/5/QtPrintSupport lib_x64/QtPrintSupport.framework/Versions/5/QtPrintSupport $qt_base2/lib/QtPrintSupport.framework/Versions/5/QtPrintSupport
lipo -create -output lib/QtGui.framework/Versions/5/QtGui lib_x64/QtGui.framework/Versions/5/QtGui $qt_base2/lib/QtGui.framework/Versions/5/QtGui
lipo -create -output lib/QtDBus.framework/Versions/5/QtDBus lib_x64/QtDBus.framework/Versions/5/QtDBus $qt_base2/lib/QtDBus.framework/Versions/5/QtDBus
lipo -create -output lib/QtCharts.framework/Versions/5/QtCharts lib_x64/QtCharts.framework/Versions/5/QtCharts $qt_base2/lib/QtCharts.framework/Versions/5/QtCharts
lipo -create -output lib/QtWebSockets.framework/Versions/5/QtWebSockets lib_x64/QtWebSockets.framework/Versions/5/QtWebSockets $qt_base2/lib/QtWebSockets.framework/Versions/5/QtWebSockets
lipo -create -output lib/QtQuick3DUtils.framework/Versions/5/QtQuick3DUtils lib_x64/QtQuick3DUtils.framework/Versions/5/QtQuick3DUtils $qt_base2/lib/QtQuick3DUtils.framework/Versions/5/QtQuick3DUtils
lipo -create -output lib/QtQuickTemplates2.framework/Versions/5/QtQuickTemplates2 lib_x64/QtQuickTemplates2.framework/Versions/5/QtQuickTemplates2 $qt_base2/lib/QtQuickTemplates2.framework/Versions/5/QtQuickTemplates2
lipo -create -output lib/QtScript.framework/Versions/5/QtScript lib_x64/QtScript.framework/Versions/5/QtScript $qt_base2/lib/QtScript.framework/Versions/5/QtScript
lipo -create -output lib/QtPositioningQuick.framework/Versions/5/QtPositioningQuick lib_x64/QtPositioningQuick.framework/Versions/5/QtPositioningQuick $qt_base2/lib/QtPositioningQuick.framework/Versions/5/QtPositioningQuick
lipo -create -output lib/Qt3DCore.framework/Versions/5/Qt3DCore lib_x64/Qt3DCore.framework/Versions/5/Qt3DCore $qt_base2/lib/Qt3DCore.framework/Versions/5/Qt3DCore
lipo -create -output lib/QtLocation.framework/Versions/5/QtLocation lib_x64/QtLocation.framework/Versions/5/QtLocation $qt_base2/lib/QtLocation.framework/Versions/5/QtLocation
lipo -create -output lib/QtXml.framework/Versions/5/QtXml lib_x64/QtXml.framework/Versions/5/QtXml $qt_base2/lib/QtXml.framework/Versions/5/QtXml
lipo -create -output lib/QtSerialPort.framework/Versions/5/QtSerialPort lib_x64/QtSerialPort.framework/Versions/5/QtSerialPort $qt_base2/lib/QtSerialPort.framework/Versions/5/QtSerialPort
lipo -create -output lib/QtWebView.framework/Versions/5/QtWebView lib_x64/QtWebView.framework/Versions/5/QtWebView $qt_base2/lib/QtWebView.framework/Versions/5/QtWebView
lipo -create -output lib/QtQuick.framework/Versions/5/QtQuick lib_x64/QtQuick.framework/Versions/5/QtQuick $qt_base2/lib/QtQuick.framework/Versions/5/QtQuick
lipo -create -output lib/QtScxml.framework/Versions/5/QtScxml lib_x64/QtScxml.framework/Versions/5/QtScxml $qt_base2/lib/QtScxml.framework/Versions/5/QtScxml
lipo -create -output lib/QtCore.framework/Versions/5/QtCore lib_x64/QtCore.framework/Versions/5/QtCore $qt_base2/lib/QtCore.framework/Versions/5/QtCore
lipo -create -output lib/QtQml.framework/Versions/5/QtQml lib_x64/QtQml.framework/Versions/5/QtQml $qt_base2/lib/QtQml.framework/Versions/5/QtQml
lipo -create -output lib/Qt3DExtras.framework/Versions/5/Qt3DExtras lib_x64/Qt3DExtras.framework/Versions/5/Qt3DExtras $qt_base2/lib/Qt3DExtras.framework/Versions/5/Qt3DExtras
lipo -create -output lib/QtWebChannel.framework/Versions/5/QtWebChannel lib_x64/QtWebChannel.framework/Versions/5/QtWebChannel $qt_base2/lib/QtWebChannel.framework/Versions/5/QtWebChannel
lipo -create -output lib/QtMultimedia.framework/Versions/5/QtMultimedia lib_x64/QtMultimedia.framework/Versions/5/QtMultimedia $qt_base2/lib/QtMultimedia.framework/Versions/5/QtMultimedia
lipo -create -output lib/QtQmlWorkerScript.framework/Versions/5/QtQmlWorkerScript lib_x64/QtQmlWorkerScript.framework/Versions/5/QtQmlWorkerScript $qt_base2/lib/QtQmlWorkerScript.framework/Versions/5/QtQmlWorkerScript
lipo -create -output lib/QtVirtualKeyboard.framework/Versions/5/QtVirtualKeyboard lib_x64/QtVirtualKeyboard.framework/Versions/5/QtVirtualKeyboard $qt_base2/lib/QtVirtualKeyboard.framework/Versions/5/QtVirtualKeyboard
lipo -create -output lib/QtPurchasing.framework/Versions/5/QtPurchasing lib_x64/QtPurchasing.framework/Versions/5/QtPurchasing $qt_base2/lib/QtPurchasing.framework/Versions/5/QtPurchasing
lipo -create -output lib/QtOpenGL.framework/Versions/5/QtOpenGL lib_x64/QtOpenGL.framework/Versions/5/QtOpenGL $qt_base2/lib/QtOpenGL.framework/Versions/5/QtOpenGL
lipo -create -output lib/Qt3DQuick.framework/Versions/5/Qt3DQuick lib_x64/Qt3DQuick.framework/Versions/5/Qt3DQuick $qt_base2/lib/Qt3DQuick.framework/Versions/5/Qt3DQuick
lipo -create -output lib/QtMacExtras.framework/Versions/5/QtMacExtras lib_x64/QtMacExtras.framework/Versions/5/QtMacExtras $qt_base2/lib/QtMacExtras.framework/Versions/5/QtMacExtras
lipo -create -output lib/QtTest.framework/Versions/5/QtTest lib_x64/QtTest.framework/Versions/5/QtTest $qt_base2/lib/QtTest.framework/Versions/5/QtTest
lipo -create -output lib/QtWidgets.framework/Versions/5/QtWidgets lib_x64/QtWidgets.framework/Versions/5/QtWidgets $qt_base2/lib/QtWidgets.framework/Versions/5/QtWidgets
lipo -create -output lib/QtPositioning.framework/Versions/5/QtPositioning lib_x64/QtPositioning.framework/Versions/5/QtPositioning $qt_base2/lib/QtPositioning.framework/Versions/5/QtPositioning
lipo -create -output lib/QtBluetooth.framework/Versions/5/QtBluetooth lib_x64/QtBluetooth.framework/Versions/5/QtBluetooth $qt_base2/lib/QtBluetooth.framework/Versions/5/QtBluetooth
lipo -create -output lib/QtQuick3D.framework/Versions/5/QtQuick3D lib_x64/QtQuick3D.framework/Versions/5/QtQuick3D $qt_base2/lib/QtQuick3D.framework/Versions/5/QtQuick3D
lipo -create -output lib/Qt3DLogic.framework/Versions/5/Qt3DLogic lib_x64/Qt3DLogic.framework/Versions/5/Qt3DLogic $qt_base2/lib/Qt3DLogic.framework/Versions/5/Qt3DLogic
lipo -create -output lib/QtQuickShapes.framework/Versions/5/QtQuickShapes lib_x64/QtQuickShapes.framework/Versions/5/QtQuickShapes $qt_base2/lib/QtQuickShapes.framework/Versions/5/QtQuickShapes
lipo -create -output lib/QtQuickTest.framework/Versions/5/QtQuickTest lib_x64/QtQuickTest.framework/Versions/5/QtQuickTest $qt_base2/lib/QtQuickTest.framework/Versions/5/QtQuickTest
lipo -create -output lib/QtNetwork.framework/Versions/5/QtNetwork lib_x64/QtNetwork.framework/Versions/5/QtNetwork $qt_base2/lib/QtNetwork.framework/Versions/5/QtNetwork
lipo -create -output lib/QtXmlPatterns.framework/Versions/5/QtXmlPatterns lib_x64/QtXmlPatterns.framework/Versions/5/QtXmlPatterns $qt_base2/lib/QtXmlPatterns.framework/Versions/5/QtXmlPatterns
lipo -create -output lib/QtSvg.framework/Versions/5/QtSvg lib_x64/QtSvg.framework/Versions/5/QtSvg $qt_base2/lib/QtSvg.framework/Versions/5/QtSvg
lipo -create -output lib/QtDesignerComponents.framework/Versions/5/QtDesignerComponents lib_x64/QtDesignerComponents.framework/Versions/5/QtDesignerComponents $qt_base2/lib/QtDesignerComponents.framework/Versions/5/QtDesignerComponents
lipo -create -output lib/QtMultimediaWidgets.framework/Versions/5/QtMultimediaWidgets lib_x64/QtMultimediaWidgets.framework/Versions/5/QtMultimediaWidgets $qt_base2/lib/QtMultimediaWidgets.framework/Versions/5/QtMultimediaWidgets
lipo -create -output lib/QtQmlModels.framework/Versions/5/QtQmlModels lib_x64/QtQmlModels.framework/Versions/5/QtQmlModels $qt_base2/lib/QtQmlModels.framework/Versions/5/QtQmlModels
lipo -create -output lib/Qt3DQuickAnimation.framework/Versions/5/Qt3DQuickAnimation lib_x64/Qt3DQuickAnimation.framework/Versions/5/Qt3DQuickAnimation $qt_base2/lib/Qt3DQuickAnimation.framework/Versions/5/Qt3DQuickAnimation
lipo -create -output lib/QtSensors.framework/Versions/5/QtSensors lib_x64/QtSensors.framework/Versions/5/QtSensors $qt_base2/lib/QtSensors.framework/Versions/5/QtSensors
lipo -create -output lib/Qt3DAnimation.framework/Versions/5/Qt3DAnimation lib_x64/Qt3DAnimation.framework/Versions/5/Qt3DAnimation $qt_base2/lib/Qt3DAnimation.framework/Versions/5/Qt3DAnimation
lipo -create -output lib/QtTextToSpeech.framework/Versions/5/QtTextToSpeech lib_x64/QtTextToSpeech.framework/Versions/5/QtTextToSpeech $qt_base2/lib/QtTextToSpeech.framework/Versions/5/QtTextToSpeech
lipo -create -output lib/QtGamepad.framework/Versions/5/QtGamepad lib_x64/QtGamepad.framework/Versions/5/QtGamepad $qt_base2/lib/QtGamepad.framework/Versions/5/QtGamepad
lipo -create -output lib/QtSerialBus.framework/Versions/5/QtSerialBus lib_x64/QtSerialBus.framework/Versions/5/QtSerialBus $qt_base2/lib/QtSerialBus.framework/Versions/5/QtSerialBus
lipo -create -output lib/QtSql.framework/Versions/5/QtSql lib_x64/QtSql.framework/Versions/5/QtSql $qt_base2/lib/QtSql.framework/Versions/5/QtSql
lipo -create -output lib/QtConcurrent.framework/Versions/5/QtConcurrent lib_x64/QtConcurrent.framework/Versions/5/QtConcurrent $qt_base2/lib/QtConcurrent.framework/Versions/5/QtConcurrent
lipo -create -output plugins/platforminputcontexts/libqtvirtualkeyboardplugin.dylib $qt_base2/plugins/platforminputcontexts/libqtvirtualkeyboardplugin.dylib plugins_x64/platforminputcontexts/libqtvirtualkeyboardplugin.dylib
lipo -create -output plugins/mediaservice/libqavfmediaplayer.dylib $qt_base2/plugins/mediaservice/libqavfmediaplayer.dylib plugins_x64/mediaservice/libqavfmediaplayer.dylib
lipo -create -output plugins/mediaservice/libqtmedia_audioengine.dylib $qt_base2/plugins/mediaservice/libqtmedia_audioengine.dylib plugins_x64/mediaservice/libqtmedia_audioengine.dylib
lipo -create -output plugins/mediaservice/libqavfcamera.dylib $qt_base2/plugins/mediaservice/libqavfcamera.dylib plugins_x64/mediaservice/libqavfcamera.dylib
lipo -create -output plugins/renderers/libopenglrenderer.dylib $qt_base2/plugins/renderers/libopenglrenderer.dylib plugins_x64/renderers/libopenglrenderer.dylib
lipo -create -output plugins/sensors/libqtsensors_generic.dylib $qt_base2/plugins/sensors/libqtsensors_generic.dylib plugins_x64/sensors/libqtsensors_generic.dylib
lipo -create -output plugins/sensors/libqtsensors_ios.dylib $qt_base2/plugins/sensors/libqtsensors_ios.dylib plugins_x64/sensors/libqtsensors_ios.dylib
lipo -create -output plugins/geoservices/libqtgeoservices_esri.dylib $qt_base2/plugins/geoservices/libqtgeoservices_esri.dylib plugins_x64/geoservices/libqtgeoservices_esri.dylib
lipo -create -output plugins/geoservices/libqtgeoservices_mapboxgl.dylib $qt_base2/plugins/geoservices/libqtgeoservices_mapboxgl.dylib plugins_x64/geoservices/libqtgeoservices_mapboxgl.dylib
lipo -create -output plugins/geoservices/libqtgeoservices_nokia.dylib $qt_base2/plugins/geoservices/libqtgeoservices_nokia.dylib plugins_x64/geoservices/libqtgeoservices_nokia.dylib
lipo -create -output plugins/geoservices/libqtgeoservices_itemsoverlay.dylib $qt_base2/plugins/geoservices/libqtgeoservices_itemsoverlay.dylib plugins_x64/geoservices/libqtgeoservices_itemsoverlay.dylib
lipo -create -output plugins/geoservices/libqtgeoservices_osm.dylib $qt_base2/plugins/geoservices/libqtgeoservices_osm.dylib plugins_x64/geoservices/libqtgeoservices_osm.dylib
lipo -create -output plugins/geoservices/libqtgeoservices_mapbox.dylib $qt_base2/plugins/geoservices/libqtgeoservices_mapbox.dylib plugins_x64/geoservices/libqtgeoservices_mapbox.dylib
lipo -create -output plugins/sceneparsers/libgltfsceneexport.dylib $qt_base2/plugins/sceneparsers/libgltfsceneexport.dylib plugins_x64/sceneparsers/libgltfsceneexport.dylib
lipo -create -output plugins/sceneparsers/libgltfsceneimport.dylib $qt_base2/plugins/sceneparsers/libgltfsceneimport.dylib plugins_x64/sceneparsers/libgltfsceneimport.dylib
lipo -create -output plugins/platforms/libqwebgl.dylib $qt_base2/plugins/platforms/libqwebgl.dylib plugins_x64/platforms/libqwebgl.dylib
lipo -create -output plugins/platforms/libqoffscreen.dylib $qt_base2/plugins/platforms/libqoffscreen.dylib plugins_x64/platforms/libqoffscreen.dylib
lipo -create -output plugins/platforms/libqminimal.dylib $qt_base2/plugins/platforms/libqminimal.dylib plugins_x64/platforms/libqminimal.dylib
lipo -create -output plugins/platforms/libqcocoa.dylib $qt_base2/plugins/platforms/libqcocoa.dylib plugins_x64/platforms/libqcocoa.dylib
lipo -create -output plugins/qmltooling/libqmldbg_messages.dylib $qt_base2/plugins/qmltooling/libqmldbg_messages.dylib plugins_x64/qmltooling/libqmldbg_messages.dylib
lipo -create -output plugins/qmltooling/libqmldbg_inspector.dylib $qt_base2/plugins/qmltooling/libqmldbg_inspector.dylib plugins_x64/qmltooling/libqmldbg_inspector.dylib
lipo -create -output plugins/qmltooling/libqmldbg_quickprofiler.dylib $qt_base2/plugins/qmltooling/libqmldbg_quickprofiler.dylib plugins_x64/qmltooling/libqmldbg_quickprofiler.dylib
lipo -create -output plugins/qmltooling/libqmldbg_tcp.dylib $qt_base2/plugins/qmltooling/libqmldbg_tcp.dylib plugins_x64/qmltooling/libqmldbg_tcp.dylib
lipo -create -output plugins/qmltooling/libqmldbg_profiler.dylib $qt_base2/plugins/qmltooling/libqmldbg_profiler.dylib plugins_x64/qmltooling/libqmldbg_profiler.dylib
lipo -create -output plugins/qmltooling/libqmldbg_preview.dylib $qt_base2/plugins/qmltooling/libqmldbg_preview.dylib plugins_x64/qmltooling/libqmldbg_preview.dylib
lipo -create -output plugins/qmltooling/libqmldbg_nativedebugger.dylib $qt_base2/plugins/qmltooling/libqmldbg_nativedebugger.dylib plugins_x64/qmltooling/libqmldbg_nativedebugger.dylib
lipo -create -output plugins/qmltooling/libqmldbg_local.dylib $qt_base2/plugins/qmltooling/libqmldbg_local.dylib plugins_x64/qmltooling/libqmldbg_local.dylib
lipo -create -output plugins/qmltooling/libqmldbg_server.dylib $qt_base2/plugins/qmltooling/libqmldbg_server.dylib plugins_x64/qmltooling/libqmldbg_server.dylib
lipo -create -output plugins/qmltooling/libqmldbg_native.dylib $qt_base2/plugins/qmltooling/libqmldbg_native.dylib plugins_x64/qmltooling/libqmldbg_native.dylib
lipo -create -output plugins/qmltooling/libqmldbg_debugger.dylib $qt_base2/plugins/qmltooling/libqmldbg_debugger.dylib plugins_x64/qmltooling/libqmldbg_debugger.dylib
lipo -create -output plugins/platformthemes/libqxdgdesktopportal.dylib $qt_base2/plugins/platformthemes/libqxdgdesktopportal.dylib plugins_x64/platformthemes/libqxdgdesktopportal.dylib
lipo -create -output plugins/assetimporters/libuip.dylib $qt_base2/plugins/assetimporters/libuip.dylib plugins_x64/assetimporters/libuip.dylib
lipo -create -output plugins/assetimporters/libassimp.dylib $qt_base2/plugins/assetimporters/libassimp.dylib plugins_x64/assetimporters/libassimp.dylib
lipo -create -output plugins/position/libqtposition_serialnmea.dylib $qt_base2/plugins/position/libqtposition_serialnmea.dylib plugins_x64/position/libqtposition_serialnmea.dylib
lipo -create -output plugins/position/libqtposition_positionpoll.dylib $qt_base2/plugins/position/libqtposition_positionpoll.dylib plugins_x64/position/libqtposition_positionpoll.dylib
lipo -create -output plugins/position/libqtposition_cl.dylib $qt_base2/plugins/position/libqtposition_cl.dylib plugins_x64/position/libqtposition_cl.dylib
lipo -create -output plugins/printsupport/libcocoaprintersupport.dylib $qt_base2/plugins/printsupport/libcocoaprintersupport.dylib plugins_x64/printsupport/libcocoaprintersupport.dylib
lipo -create -output plugins/sensorgestures/libqtsensorgestures_shakeplugin.dylib $qt_base2/plugins/sensorgestures/libqtsensorgestures_shakeplugin.dylib plugins_x64/sensorgestures/libqtsensorgestures_shakeplugin.dylib
lipo -create -output plugins/sensorgestures/libqtsensorgestures_plugin.dylib $qt_base2/plugins/sensorgestures/libqtsensorgestures_plugin.dylib plugins_x64/sensorgestures/libqtsensorgestures_plugin.dylib
# lipo -create -output plugins/webview/libqtwebview_webengine.dylib $qt_base2/plugins/webview/libqtwebview_webengine.dylib plugins_x64/webview/libqtwebview_webengine.dylib
lipo -create -output plugins/webview/libqtwebview_darwin.dylib $qt_base2/plugins/webview/libqtwebview_darwin.dylib plugins_x64/webview/libqtwebview_darwin.dylib
lipo -create -output plugins/geometryloaders/libdefaultgeometryloader.dylib $qt_base2/plugins/geometryloaders/libdefaultgeometryloader.dylib plugins_x64/geometryloaders/libdefaultgeometryloader.dylib
lipo -create -output plugins/geometryloaders/libgltfgeometryloader.dylib $qt_base2/plugins/geometryloaders/libgltfgeometryloader.dylib plugins_x64/geometryloaders/libgltfgeometryloader.dylib
lipo -create -output plugins/styles/libqmacstyle.dylib $qt_base2/plugins/styles/libqmacstyle.dylib plugins_x64/styles/libqmacstyle.dylib
lipo -create -output plugins/audio/libqtaudio_coreaudio.dylib $qt_base2/plugins/audio/libqtaudio_coreaudio.dylib plugins_x64/audio/libqtaudio_coreaudio.dylib
lipo -create -output plugins/canbus/libqttinycanbus.dylib $qt_base2/plugins/canbus/libqttinycanbus.dylib plugins_x64/canbus/libqttinycanbus.dylib
lipo -create -output plugins/canbus/libqtpassthrucanbus.dylib $qt_base2/plugins/canbus/libqtpassthrucanbus.dylib plugins_x64/canbus/libqtpassthrucanbus.dylib
lipo -create -output plugins/canbus/libqtvirtualcanbus.dylib $qt_base2/plugins/canbus/libqtvirtualcanbus.dylib plugins_x64/canbus/libqtvirtualcanbus.dylib
lipo -create -output plugins/canbus/libqtpeakcanbus.dylib $qt_base2/plugins/canbus/libqtpeakcanbus.dylib plugins_x64/canbus/libqtpeakcanbus.dylib
lipo -create -output plugins/bearer/libqgenericbearer.dylib $qt_base2/plugins/bearer/libqgenericbearer.dylib plugins_x64/bearer/libqgenericbearer.dylib
lipo -create -output plugins/iconengines/libqsvgicon.dylib $qt_base2/plugins/iconengines/libqsvgicon.dylib plugins_x64/iconengines/libqsvgicon.dylib
lipo -create -output plugins/sqldrivers/libqsqlite.dylib $qt_base2/plugins/sqldrivers/libqsqlite.dylib plugins_x64/sqldrivers/libqsqlite.dylib
lipo -create -output plugins/imageformats/libqgif.dylib $qt_base2/plugins/imageformats/libqgif.dylib plugins_x64/imageformats/libqgif.dylib
lipo -create -output plugins/imageformats/libqwbmp.dylib $qt_base2/plugins/imageformats/libqwbmp.dylib plugins_x64/imageformats/libqwbmp.dylib
lipo -create -output plugins/imageformats/libqwebp.dylib $qt_base2/plugins/imageformats/libqwebp.dylib plugins_x64/imageformats/libqwebp.dylib
lipo -create -output plugins/imageformats/libqico.dylib $qt_base2/plugins/imageformats/libqico.dylib plugins_x64/imageformats/libqico.dylib
lipo -create -output plugins/imageformats/libqmacheif.dylib $qt_base2/plugins/imageformats/libqmacheif.dylib plugins_x64/imageformats/libqmacheif.dylib
lipo -create -output plugins/imageformats/libqjpeg.dylib $qt_base2/plugins/imageformats/libqjpeg.dylib plugins_x64/imageformats/libqjpeg.dylib
lipo -create -output plugins/imageformats/libqtiff.dylib $qt_base2/plugins/imageformats/libqtiff.dylib plugins_x64/imageformats/libqtiff.dylib
lipo -create -output plugins/imageformats/libqsvg.dylib $qt_base2/plugins/imageformats/libqsvg.dylib plugins_x64/imageformats/libqsvg.dylib
# lipo -create -output plugins/imageformats/libqpdf.dylib $qt_base2/plugins/imageformats/libqpdf.dylib plugins_x64/imageformats/libqpdf.dylib
lipo -create -output plugins/imageformats/libqicns.dylib $qt_base2/plugins/imageformats/libqicns.dylib plugins_x64/imageformats/libqicns.dylib
lipo -create -output plugins/imageformats/libqtga.dylib $qt_base2/plugins/imageformats/libqtga.dylib plugins_x64/imageformats/libqtga.dylib
lipo -create -output plugins/imageformats/libqmacjp2.dylib $qt_base2/plugins/imageformats/libqmacjp2.dylib plugins_x64/imageformats/libqmacjp2.dylib
lipo -create -output plugins/designer/libqquickwidget.dylib $qt_base2/plugins/designer/libqquickwidget.dylib plugins_x64/designer/libqquickwidget.dylib
# lipo -create -output plugins/designer/libqwebengineview.dylib $qt_base2/plugins/designer/libqwebengineview.dylib plugins_x64/designer/libqwebengineview.dylib
lipo -create -output plugins/texttospeech/libqtexttospeech_speechosx.dylib $qt_base2/plugins/texttospeech/libqtexttospeech_speechosx.dylib plugins_x64/texttospeech/libqtexttospeech_speechosx.dylib
lipo -create -output plugins/generic/libqtuiotouchplugin.dylib $qt_base2/plugins/generic/libqtuiotouchplugin.dylib plugins_x64/generic/libqtuiotouchplugin.dylib
lipo -create -output plugins/renderplugins/libscene2d.dylib $qt_base2/plugins/renderplugins/libscene2d.dylib plugins_x64/renderplugins/libscene2d.dylib
lipo -create -output plugins/playlistformats/libqtmultimedia_m3u.dylib $qt_base2/plugins/playlistformats/libqtmultimedia_m3u.dylib plugins_x64/playlistformats/libqtmultimedia_m3u.dylib
lipo -create -output plugins/gamepads/libdarwingamepad.dylib $qt_base2/plugins/gamepads/libdarwingamepad.dylib plugins_x64/gamepads/libdarwingamepad.dylib
lipo -create -output plugins/virtualkeyboard/libqtvirtualkeyboard_thai.dylib $qt_base2/plugins/virtualkeyboard/libqtvirtualkeyboard_thai.dylib plugins_x64/virtualkeyboard/libqtvirtualkeyboard_thai.dylib
lipo -create -output plugins/virtualkeyboard/libqtvirtualkeyboard_openwnn.dylib $qt_base2/plugins/virtualkeyboard/libqtvirtualkeyboard_openwnn.dylib plugins_x64/virtualkeyboard/libqtvirtualkeyboard_openwnn.dylib
lipo -create -output plugins/virtualkeyboard/libqtvirtualkeyboard_hangul.dylib $qt_base2/plugins/virtualkeyboard/libqtvirtualkeyboard_hangul.dylib plugins_x64/virtualkeyboard/libqtvirtualkeyboard_hangul.dylib
lipo -create -output plugins/virtualkeyboard/libqtvirtualkeyboard_pinyin.dylib $qt_base2/plugins/virtualkeyboard/libqtvirtualkeyboard_pinyin.dylib plugins_x64/virtualkeyboard/libqtvirtualkeyboard_pinyin.dylib
lipo -create -output plugins/virtualkeyboard/libqtvirtualkeyboard_tcime.dylib $qt_base2/plugins/virtualkeyboard/libqtvirtualkeyboard_tcime.dylib plugins_x64/virtualkeyboard/libqtvirtualkeyboard_tcime.dylib

popd