/***************************************************************************
                         qgspointcloudattribute.cpp
                         -----------------------
    begin                : October 2020
    copyright            : (C) 2020 by Peter Petrik
    email                : zilolv at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgis.h"
#include "qgspointcloudattribute.h"

QgsPointCloudAttribute::QgsPointCloudAttribute() = default;

QgsPointCloudAttribute::QgsPointCloudAttribute( const QString &name, DataType type )
  : mName( name )
  , mType( type )
{
  updateSize();
}

QVariant::Type QgsPointCloudAttribute::variantType() const
{
  switch ( mType )
  {
    case DataType::Char:
    case DataType::Short:
    case DataType::UShort:
    case DataType::Int32:
      return QVariant::Int;

    case DataType::Float:
    case DataType::Double:
      return QVariant::Double;
  }
  return QVariant::Invalid;
}

QString QgsPointCloudAttribute::displayType() const
{
  switch ( mType )
  {
    case DataType::Char:
      return QObject::tr( "Character" );
    case DataType::Short:
      return QObject::tr( "Short" );
    case DataType::UShort:
      return QObject::tr( "Unsigned Short" );
    case DataType::Float:
      return QObject::tr( "Float" );
    case DataType::Int32:
      return QObject::tr( "Integer" );
    case DataType::Double:
      return QObject::tr( "Double" );
  }
  return QString();
}

bool QgsPointCloudAttribute::isNumeric( QgsPointCloudAttribute::DataType type )
{
  switch ( type )
  {
    case DataType::Char:
      return false;
    case DataType::Short:
    case DataType::UShort:
    case DataType::Float:
    case DataType::Int32:
    case DataType::Double:
      return true;
  }
  return false;
}

void QgsPointCloudAttribute::updateSize()
{
  switch ( mType )
  {
    case DataType::Char:
      mSize = 1;
      break;
    case DataType::Short:
    case DataType::UShort:
      mSize = 2;
      break;
    case DataType::Float:
      mSize = 4;
      break;
    case DataType::Int32:
      mSize = 4;
      break;
    case DataType::Double:
      mSize = 8;
      break;
  }
}

// //////////////////

QgsPointCloudAttributeCollection::QgsPointCloudAttributeCollection() = default;

QgsPointCloudAttributeCollection::QgsPointCloudAttributeCollection( const QVector<QgsPointCloudAttribute> &attributes )
{
  mAttributes.reserve( attributes.size() );
  for ( const QgsPointCloudAttribute &attribute : attributes )
  {
    push_back( attribute );
  }
}

void QgsPointCloudAttributeCollection::push_back( const QgsPointCloudAttribute &attribute )
{
  mCachedAttributes.insert( attribute.name(), CachedAttributeData( mAttributes.size(), mSize ) );
  mAttributes.push_back( attribute );
  mSize += attribute.size();
}

QVector<QgsPointCloudAttribute> QgsPointCloudAttributeCollection::attributes() const
{
  return mAttributes;
}

const QgsPointCloudAttribute *QgsPointCloudAttributeCollection::find( const QString &attributeName, int &offset ) const
{
  const auto it = mCachedAttributes.constFind( attributeName );
  if ( it != mCachedAttributes.constEnd() )
  {
    offset = it->offset;
    return &mAttributes.at( it->index );
  }

  // not found
  return nullptr;
}

int QgsPointCloudAttributeCollection::indexOf( const QString &name ) const
{
  const auto it = mCachedAttributes.constFind( name );
  if ( it != mCachedAttributes.constEnd() )
  {
    return it->index;
  }

  // not found
  return -1;
}

QgsFields QgsPointCloudAttributeCollection::toFields() const
{
  QgsFields fields;
  for ( const QgsPointCloudAttribute &attribute : mAttributes )
  {
    fields.append( QgsField( attribute.name(), attribute.variantType(), attribute.displayType() ) );
  }
  return fields;
}

template <typename T>
void _attribute( const char *data, std::size_t offset, QgsPointCloudAttribute::DataType type, T &value )
{
  switch ( type )
  {
    case QgsPointCloudAttribute::Char:
      value = *( data + offset );
      break;

    case QgsPointCloudAttribute::Int32:
      value = *reinterpret_cast< const qint32 * >( data + offset );
      break;

    case QgsPointCloudAttribute::Short:
    {
      value = *reinterpret_cast< const short * >( data + offset );
    }
    break;

    case QgsPointCloudAttribute::UShort:
      value = *reinterpret_cast< const unsigned short * >( data + offset );
      break;

    case QgsPointCloudAttribute::Float:
      value = static_cast< T >( *reinterpret_cast< const float * >( data + offset ) );
      break;

    case QgsPointCloudAttribute::Double:
      value = *reinterpret_cast< const double * >( data + offset );
      break;
  }
}

void QgsPointCloudAttribute::getPointXYZ( const char *ptr, int i, std::size_t pointRecordSize, int xOffset, QgsPointCloudAttribute::DataType xType,
    int yOffset, QgsPointCloudAttribute::DataType yType,
    int zOffset, QgsPointCloudAttribute::DataType zType,
    const QgsVector3D &indexScale, const QgsVector3D &indexOffset, double &x, double &y, double &z )
{
  _attribute( ptr, i * pointRecordSize + xOffset, xType, x );
  x = indexOffset.x() + indexScale.x() * x;

  _attribute( ptr, i * pointRecordSize + yOffset, yType, y );
  y = indexOffset.y() + indexScale.y() * y;

  _attribute( ptr, i * pointRecordSize + zOffset, zType, z );
  z = indexOffset.z() + indexScale.z() * z;
}

QVariantMap QgsPointCloudAttribute::getAttributeMap( const char *data, std::size_t recordOffset, const QgsPointCloudAttributeCollection &attributeCollection )
{
  QVariantMap map;
  const QVector<QgsPointCloudAttribute> attributes = attributeCollection.attributes();
  for ( const QgsPointCloudAttribute &attr : attributes )
  {
    const QString attributeName = attr.name();
    int attributeOffset;
    attributeCollection.find( attributeName, attributeOffset );
    switch ( attr.type() )
    {
      case QgsPointCloudAttribute::Char:
      {
        const char value = *( data + recordOffset + attributeOffset );
        map[ attributeName ] = value;
      }
      break;

      case QgsPointCloudAttribute::Int32:
      {
        const qint32 value = *reinterpret_cast< const qint32 * >( data + recordOffset + attributeOffset );
        map[ attributeName ] = value;
      }
      break;

      case QgsPointCloudAttribute::Short:
      {
        const short value = *reinterpret_cast< const short * >( data + recordOffset + attributeOffset );
        map[ attributeName ] = value;
      }
      break;

      case QgsPointCloudAttribute::UShort:
      {
        const unsigned short value = *reinterpret_cast< const unsigned short * >( data + recordOffset + attributeOffset );
        map[ attributeName ] = value;
      }
      break;

      case QgsPointCloudAttribute::Float:
      {
        const float value = *reinterpret_cast< const float * >( data + recordOffset + attributeOffset );
        map[ attributeName ] = value;
      }
      break;

      case QgsPointCloudAttribute::Double:
      {
        const double value = *reinterpret_cast< const double * >( data + recordOffset + attributeOffset );
        map[ attributeName ] = value;
      }
      break;
    }
  }
  return map;
}
