/***************************************************************************
                         qgsmssqlfeatureiterator.h  -  description
                         -------------------
    begin                : 2011-10-08
    copyright            : (C) 2011 by Tamas Szekeres
    email                : szekerest at gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMSSQLFEATUREITERATOR_H
#define QGSMSSQLFEATUREITERATOR_H

#include "qgsmssqlgeometryparser.h"
#include "qgsfeatureiterator.h"
#include "qgsfields.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include "qgsmssqlprovider.h"

class QgsMssqlProvider;

class QgsMssqlFeatureSource final: public QgsAbstractFeatureSource
{
  public:
    explicit QgsMssqlFeatureSource( const QgsMssqlProvider *p );

    QgsFeatureIterator getFeatures( const QgsFeatureRequest &request ) override;

  private:
    QgsFields mFields;
    QgsMssqlPrimaryKeyType mPrimaryKeyType;
    QList<int> mPrimaryKeyAttrs;
    std::shared_ptr<QgsMssqlSharedData> mShared;
    long mSRId;

    /* sql geo type */
    bool mIsGeography = false;

    QString mGeometryColName;
    QString mGeometryColType;

    // current layer name
    QString mSchemaName;
    QString mTableName;

    // login
    QString mUserName;
    QString mPassword;

    // server access
    QString mService;
    QString mDatabaseName;
    QString mHost;

    // SQL statement used to limit the features retrieved
    QString mSqlWhereClause;

    bool mDisableInvalidGeometryHandling = false;

    QgsCoordinateReferenceSystem mCrs;

    // Return True if this feature source has spatial attributes.
    bool isSpatial() { return !mGeometryColName.isEmpty() || !mGeometryColType.isEmpty(); }

    friend class QgsMssqlFeatureIterator;
    friend class QgsMssqlExpressionCompiler;
};

class QgsMssqlFeatureIterator final: public QgsAbstractFeatureIteratorFromSource<QgsMssqlFeatureSource>
{
  public:
    QgsMssqlFeatureIterator( QgsMssqlFeatureSource *source, bool ownSource, const QgsFeatureRequest &request );

    ~QgsMssqlFeatureIterator() override;

    bool rewind() override;
    bool close() override;

  protected:
    bool fetchFeature( QgsFeature &feature ) override;
    bool nextFeatureFilterExpression( QgsFeature &f ) override;

  private:
    void BuildStatement( const QgsFeatureRequest &request );
    QString whereClauseFid( QgsFeatureId featureId );

  private:

    bool prepareOrderBy( const QList<QgsFeatureRequest::OrderByClause> &orderBys ) override;

    double validLat( double latitude ) const;
    double validLon( double longitude ) const;

    // The current database
    QSqlDatabase mDatabase;

    // The current sql query
    std::unique_ptr< QSqlQuery > mQuery;

    // The current sql statement
    QString mStatement;
    QString mOrderByClause;

    QString mFallbackStatement;

    // List of attribute indices to fetch with nextFeature calls
    QgsAttributeList mAttributesToFetch;

    // for parsing sql geometries
    QgsMssqlGeometryParser mParser;

    bool mExpressionCompiled = false;
    bool mOrderByCompiled = false;
    bool mDisableInvalidGeometryHandling = false;

    QgsCoordinateTransform mTransform;
    QgsRectangle mFilterRect;
    QgsGeometry mDistanceWithinGeom;
    std::unique_ptr< QgsGeometryEngine > mDistanceWithinEngine;
};

#endif // QGSMSSQLFEATUREITERATOR_H
