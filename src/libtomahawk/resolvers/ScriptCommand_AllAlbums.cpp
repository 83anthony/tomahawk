/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptCommand_AllAlbums.h"

#include "utils/Logger.h"

#include "Album.h"
#include "Artist.h"
#include "ScriptAccount.h"
#include "PlaylistEntry.h"
#include "ScriptCollection.h"
#include "ScriptJob.h"
#include "ScriptCommand_AllArtists.h"

using namespace Tomahawk;

ScriptCommand_AllAlbums::ScriptCommand_AllAlbums( const Tomahawk::collection_ptr& collection,
                                                  const Tomahawk::artist_ptr& artist,
                                                  QObject* parent )
    : ScriptCommand( parent )
    , m_collection( collection )
    , m_artist( artist )
{
}


void
ScriptCommand_AllAlbums::enqueue()
{
    Tomahawk::ScriptCollection* collection = qobject_cast< Tomahawk::ScriptCollection* >( m_collection.data() );
    if ( collection == 0 )
    {
        emit albums( QList< Tomahawk::album_ptr >() );
        return;
    }

    exec();
}


void
ScriptCommand_AllAlbums::setFilter( const QString& filter )
{
    m_filter = filter;
}


void
ScriptCommand_AllAlbums::exec()
{
    Tomahawk::ScriptCollection* collection = qobject_cast< Tomahawk::ScriptCollection* >( m_collection.data() );
    if ( collection == 0 )
    {
        reportFailure();
        return;
    }

    ScriptJob* job;
    if ( m_artist )
    {
        QVariantMap arguments;
        arguments[ "artist" ] = m_artist->name();
        job = collection->scriptObject()->invoke( "artistAlbums", arguments );
    }
    else
    {
        job = collection->scriptObject()->invoke( "albums" );
    }

    connect( job, SIGNAL( done( QVariantMap ) ), SLOT( onAlbumsJobDone( QVariantMap ) ), Qt::QueuedConnection );
    job->start();
}


void
ScriptCommand_AllAlbums::reportFailure()
{
    if ( m_artist )
        tDebug() << Q_FUNC_INFO << "for collection" << m_collection->name() << "and artist" << m_artist->name();

    emit albums( QList< Tomahawk::album_ptr >() );
    emit done();
}


void
ScriptCommand_AllAlbums::onAlbumsJobDone(const QVariantMap& result)
{
    ScriptJob* job = qobject_cast< ScriptJob* >( sender() );
    Q_ASSERT( job );

    if ( job->error() )
    {
        reportFailure();
        return;
    }

    QList< Tomahawk::artist_ptr > resultArtists;
    if ( result["artists"].toList().length() > 0 )
    {
        resultArtists = ScriptCommand_AllArtists::parseArtistVariantList( result[ "artists" ].toList() );
    }
    else
    {
        resultArtists << m_artist;
    }

    QList< Tomahawk::album_ptr > a = parseAlbumVariantList( resultArtists, result[ "albums" ].toList() );
    if ( m_filter.isEmpty() )
        emit albums( a );
    else
    {
        QList< Tomahawk::album_ptr > filtered;
        foreach( const Tomahawk::album_ptr& album, a )
        {
            if( album->name().toLower().contains( m_filter.toLower() ) ||
                album->artist()->name().toLower().contains( m_filter.toLower() ) )
                filtered.append( album );
        }
        emit albums( filtered );
    }
    emit done();

    job->deleteLater();
}


QList< Tomahawk::album_ptr >
ScriptCommand_AllAlbums::parseAlbumVariantList( const QList< Tomahawk::artist_ptr >& artists, const QVariantList& reslist )
{
    QList< Tomahawk::album_ptr > results;

    if (artists.length() != 1 && reslist.length() != artists.length())
    {
        tLog() << "artists" << artists.length();
        tLog() << "albums" << reslist.length();
        Q_ASSERT(false);
        tLog() << "Got invalid collection albums response where artists and albums don't match";
        return results;
    }

    bool useArtistList = ( artists.length() > 1 );
    for( int i=0; i<reslist.length(); i++ )
    {
        const QString val = reslist[ i ].toString();
        if ( val.trimmed().isEmpty() )
            continue;

        if ( useArtistList )
        {
            results << Tomahawk::Album::get( artists[ i ], val, false );
        }
        else
        {
            results << Tomahawk::Album::get( artists[ 0 ], val, false );
        }
    }

    return results;
}
