/***************************************************************************
                         qgsalgorithmlayoutatlastopdf.cpp
                         ---------------------
    begin                : August 2020
    copyright            : (C) 2020 by Mathieu Pellerin
    email                : nirvn dot asia at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsalgorithmlayoutatlastopdf.h"
#include "qgslayout.h"
#include "qgslayoutatlas.h"
#include "qgslayoutitemmap.h"
#include "qgslayoututils.h"
#include "qgsprintlayout.h"
#include "qgsprocessingoutputs.h"
#include "qgslayoutexporter.h"

///@cond PRIVATE

QString QgsLayoutAtlasToPdfAlgorithm::name() const
{
  return QStringLiteral( "atlaslayouttopdf" );
}

QString QgsLayoutAtlasToPdfAlgorithm::displayName() const
{
  return QObject::tr( "Export atlas layout as PDF" );
}

QStringList QgsLayoutAtlasToPdfAlgorithm::tags() const
{
  return QObject::tr( "layout,atlas,composer,composition,save" ).split( ',' );
}

QString QgsLayoutAtlasToPdfAlgorithm::group() const
{
  return QObject::tr( "Cartography" );
}

QString QgsLayoutAtlasToPdfAlgorithm::groupId() const
{
  return QStringLiteral( "cartography" );
}

QString QgsLayoutAtlasToPdfAlgorithm::shortDescription() const
{
  return QObject::tr( "Exports an atlas layout as a PDF." );
}

QString QgsLayoutAtlasToPdfAlgorithm::shortHelpString() const
{
  return QObject::tr( "This algorithm outputs an atlas layout as a PDF file.\n\n"
                      "If a coverage layer is set, the selected layout's atlas settings exposed in this algorithm "
                      "will be overwritten. In this case, an empty filter or sort by expression will turn those "
                      "settings off." );
}

void QgsLayoutAtlasToPdfAlgorithm::initAlgorithm( const QVariantMap & )
{
  addParameter( new QgsProcessingParameterLayout( QStringLiteral( "LAYOUT" ), QObject::tr( "Atlas layout" ) ) );

  addParameter( new QgsProcessingParameterVectorLayer( QStringLiteral( "COVERAGE_LAYER" ), QObject::tr( "Coverage layer" ), QList< int >(), QVariant(), true ) );
  addParameter( new QgsProcessingParameterExpression( QStringLiteral( "FILTER_EXPRESSION" ), QObject::tr( "Filter expression" ), QString(), QStringLiteral( "COVERAGE_LAYER" ), true ) );
  addParameter( new QgsProcessingParameterExpression( QStringLiteral( "SORTBY_EXPRESSION" ), QObject::tr( "Sort expression" ), QString(), QStringLiteral( "COVERAGE_LAYER" ), true ) );
  addParameter( new QgsProcessingParameterBoolean( QStringLiteral( "SORTBY_REVERSE" ), QObject::tr( "Reverse sort order (used when a sort expression is provided)" ), false, true ) );

  addParameter( new QgsProcessingParameterFileDestination( QStringLiteral( "OUTPUT" ), QObject::tr( "PDF file" ), QObject::tr( "PDF Format" ) + " (*.pdf *.PDF)" ) );

  std::unique_ptr< QgsProcessingParameterMultipleLayers > layersParam = std::make_unique< QgsProcessingParameterMultipleLayers>( QStringLiteral( "LAYERS" ), QObject::tr( "Map layers to assign to unlocked map item(s)" ), QgsProcessing::TypeMapLayer, QVariant(), true );
  layersParam->setFlags( layersParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( layersParam.release() );

  std::unique_ptr< QgsProcessingParameterNumber > dpiParam = std::make_unique< QgsProcessingParameterNumber >( QStringLiteral( "DPI" ), QObject::tr( "DPI (leave blank for default layout DPI)" ), QgsProcessingParameterNumber::Double, QVariant(), true, 0 );
  dpiParam->setFlags( dpiParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( dpiParam.release() );

  std::unique_ptr< QgsProcessingParameterBoolean > forceVectorParam = std::make_unique< QgsProcessingParameterBoolean >( QStringLiteral( "FORCE_VECTOR" ), QObject::tr( "Always export as vectors" ), false );
  forceVectorParam->setFlags( forceVectorParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( forceVectorParam.release() );

  std::unique_ptr< QgsProcessingParameterBoolean > appendGeorefParam = std::make_unique< QgsProcessingParameterBoolean >( QStringLiteral( "GEOREFERENCE" ), QObject::tr( "Append georeference information" ), true );
  appendGeorefParam->setFlags( appendGeorefParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( appendGeorefParam.release() );

  std::unique_ptr< QgsProcessingParameterBoolean > exportRDFParam = std::make_unique< QgsProcessingParameterBoolean >( QStringLiteral( "INCLUDE_METADATA" ), QObject::tr( "Export RDF metadata (title, author, etc.)" ), true );
  exportRDFParam->setFlags( exportRDFParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( exportRDFParam.release() );

  std::unique_ptr< QgsProcessingParameterBoolean > disableTiled = std::make_unique< QgsProcessingParameterBoolean >( QStringLiteral( "DISABLE_TILED" ), QObject::tr( "Disable tiled raster layer exports" ), false );
  disableTiled->setFlags( disableTiled->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( disableTiled.release() );

  std::unique_ptr< QgsProcessingParameterBoolean > simplify = std::make_unique< QgsProcessingParameterBoolean >( QStringLiteral( "SIMPLIFY" ), QObject::tr( "Simplify geometries to reduce output file size" ), true );
  simplify->setFlags( simplify->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( simplify.release() );

  const QStringList textExportOptions
  {
    QObject::tr( "Always Export Text as Paths (Recommended)" ),
    QObject::tr( "Always Export Text as Text Objects" )
  };

  std::unique_ptr< QgsProcessingParameterEnum > textFormat = std::make_unique< QgsProcessingParameterEnum >( QStringLiteral( "TEXT_FORMAT" ), QObject::tr( "Text export" ), textExportOptions, false, 0 );
  textFormat->setFlags( textFormat->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( textFormat.release() );
}

QgsProcessingAlgorithm::Flags QgsLayoutAtlasToPdfAlgorithm::flags() const
{
  return QgsProcessingAlgorithm::flags() | FlagNoThreading;
}

QgsLayoutAtlasToPdfAlgorithm *QgsLayoutAtlasToPdfAlgorithm::createInstance() const
{
  return new QgsLayoutAtlasToPdfAlgorithm();
}

QVariantMap QgsLayoutAtlasToPdfAlgorithm::processAlgorithm( const QVariantMap &parameters, QgsProcessingContext &context, QgsProcessingFeedback *feedback )
{
  // this needs to be done in main thread, layouts are not thread safe
  QgsPrintLayout *l = parameterAsLayout( parameters, QStringLiteral( "LAYOUT" ), context );
  if ( !l )
    throw QgsProcessingException( QObject::tr( "Cannot find layout with name \"%1\"" ).arg( parameters.value( QStringLiteral( "LAYOUT" ) ).toString() ) );

  std::unique_ptr< QgsPrintLayout > layout( l->clone() );
  QgsLayoutAtlas *atlas = layout->atlas();

  QString expression, error;
  QgsVectorLayer *layer = parameterAsVectorLayer( parameters, QStringLiteral( "COVERAGE_LAYER" ), context );
  if ( layer )
  {
    atlas->setEnabled( true );
    atlas->setCoverageLayer( layer );

    expression = parameterAsString( parameters, QStringLiteral( "FILTER_EXPRESSION" ), context );
    atlas->setFilterExpression( expression, error );
    atlas->setFilterFeatures( !expression.isEmpty() && error.isEmpty() );
    if ( !expression.isEmpty() && !error.isEmpty() )
    {
      throw QgsProcessingException( QObject::tr( "Error setting atlas filter expression" ) );
    }
    error.clear();

    expression = parameterAsString( parameters, QStringLiteral( "SORTBY_EXPRESSION" ), context );
    if ( !expression.isEmpty() )
    {
      const bool sortByReverse = parameterAsBool( parameters, QStringLiteral( "SORTBY_REVERSE" ), context );
      atlas->setSortFeatures( true );
      atlas->setSortExpression( expression );
      atlas->setSortAscending( !sortByReverse );
    }
    else
    {
      atlas->setSortFeatures( false );
    }
  }
  else if ( !atlas->enabled() )
  {
    throw QgsProcessingException( QObject::tr( "Layout being export doesn't have an enabled atlas" ) );
  }

  const QgsLayoutExporter exporter( layout.get() );
  QgsLayoutExporter::PdfExportSettings settings;

  if ( parameters.value( QStringLiteral( "DPI" ) ).isValid() )
  {
    settings.dpi = parameterAsDouble( parameters, QStringLiteral( "DPI" ), context );
  }
  settings.forceVectorOutput = parameterAsBool( parameters, QStringLiteral( "FORCE_VECTOR" ), context );
  settings.appendGeoreference = parameterAsBool( parameters, QStringLiteral( "GEOREFERENCE" ), context );
  settings.exportMetadata = parameterAsBool( parameters, QStringLiteral( "INCLUDE_METADATA" ), context );
  settings.simplifyGeometries = parameterAsBool( parameters, QStringLiteral( "SIMPLIFY" ), context );
  settings.textRenderFormat = parameterAsEnum( parameters, QStringLiteral( "TEXT_FORMAT" ), context ) == 0 ? QgsRenderContext::TextFormatAlwaysOutlines : QgsRenderContext::TextFormatAlwaysText;

  if ( parameterAsBool( parameters, QStringLiteral( "DISABLE_TILED" ), context ) )
    settings.flags = settings.flags | QgsLayoutRenderContext::FlagDisableTiledRasterLayerRenders;
  else
    settings.flags = settings.flags & ~QgsLayoutRenderContext::FlagDisableTiledRasterLayerRenders;

  settings.predefinedMapScales = QgsLayoutUtils::predefinedScales( layout.get() );

  const QList< QgsMapLayer * > layers = parameterAsLayerList( parameters, QStringLiteral( "LAYERS" ), context );
  if ( layers.size() > 0 )
  {
    const QList<QGraphicsItem *> items = layout->items();
    for ( QGraphicsItem *graphicsItem : items )
    {
      QgsLayoutItem *item = dynamic_cast<QgsLayoutItem *>( graphicsItem );
      QgsLayoutItemMap *map = dynamic_cast<QgsLayoutItemMap *>( item );
      if ( map && !map->followVisibilityPreset() && !map->keepLayerSet() )
      {
        map->setKeepLayerSet( true );
        map->setLayers( layers );
      }
    }
  }

  const QString dest = parameterAsFileOutput( parameters, QStringLiteral( "OUTPUT" ), context );
  if ( atlas->updateFeatures() )
  {
    feedback->pushInfo( QObject::tr( "Exporting %n atlas feature(s)", "", atlas->count() ) );
    switch ( exporter.exportToPdf( atlas, dest, settings, error, feedback ) )
    {
      case QgsLayoutExporter::Success:
      {
        feedback->pushInfo( QObject::tr( "Successfully exported layout to %1" ).arg( QDir::toNativeSeparators( dest ) ) );
        break;
      }

      case QgsLayoutExporter::FileError:
        throw QgsProcessingException( QObject::tr( "Cannot write to %1.\n\nThis file may be open in another application." ).arg( QDir::toNativeSeparators( dest ) ) );

      case QgsLayoutExporter::PrintError:
        throw QgsProcessingException( QObject::tr( "Could not create print device." ) );

      case QgsLayoutExporter::MemoryError:
        throw QgsProcessingException( QObject::tr( "Trying to create the image "
                                      "resulted in a memory overflow.\n\n"
                                      "Please try a lower resolution or a smaller paper size." ) );

      case QgsLayoutExporter::IteratorError:
        throw QgsProcessingException( QObject::tr( "Error encountered while exporting atlas." ) );

      case QgsLayoutExporter::SvgLayerError:
      case QgsLayoutExporter::Canceled:
        // no meaning for imageexports, will not be encountered
        break;
    }
  }
  else
  {
    feedback->reportError( QObject::tr( "No atlas features found" ) );
  }

  feedback->setProgress( 100 );

  QVariantMap outputs;
  outputs.insert( QStringLiteral( "OUTPUT" ), dest );
  return outputs;
}

///@endcond

