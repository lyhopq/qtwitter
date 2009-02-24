/***************************************************************************
 *   Copyright (C) 2008-2009 by Dominik Kapusta       <d@ayoy.net>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "xmldownload.h"
#include "xmlparserdirectmsg.h"
#include "core.h"

XmlData::XmlData() :
    id(-1),
    buffer(0),
    bytearray(0)
{}

XmlData::~XmlData()
{
  clear();
}

void XmlData::assign( int newId, QBuffer *newBuffer, QByteArray *newByteArray )
{
  id = newId;
  buffer = newBuffer;
  bytearray = newByteArray;
}

void XmlData::clear()
{
  if (buffer) {
    if ( buffer->isOpen() ) {
      buffer->close();
    }
    delete buffer;
    buffer = 0;
  }
  if(bytearray) {
    delete bytearray;
    bytearray = 0;
  }
}

XmlDownload::XmlDownload( Role role, Core *coreParent, QObject *parent ) :
    HttpConnection( parent ),
    connectionRole( role ),
    statusParser(0),
    directMsgParser(0),
    core( coreParent ),
    authenticated( false )
{
  statusParser = new XmlParser( this );
  if ( connectionRole == XmlDownload::RefreshAll ) {
    directMsgParser = new XmlParserDirectMsg( this );
  }
  createConnections( core );
}

XmlDownload::Role XmlDownload::role() const
{
  return connectionRole;
}

void XmlDownload::createConnections( Core *coreParent )
{
  connect( statusParser, SIGNAL(dataParsed(QString)), this, SIGNAL(dataParsed(QString)));
  connect( this, SIGNAL(finished(XmlDownload::ContentRequested)), coreParent, SLOT(setFlag(XmlDownload::ContentRequested)) );
  connect( this, SIGNAL(errorMessage(QString)), coreParent, SIGNAL(errorMessage(QString)) );
  if ( connectionRole == Destroy ) {
    connect( statusParser, SIGNAL(newEntry(Entry*)), this, SLOT(extractId(Entry*)) );
    connect( this, SIGNAL(deleteEntry(int)), coreParent, SIGNAL(deleteEntry(int)) );
  } else {
    connect( statusParser, SIGNAL(newEntry(Entry*)), coreParent, SLOT(newEntry(Entry*)) );
    connect( statusParser, SIGNAL(newEntry(Entry*)), coreParent, SLOT(downloadOneImage(Entry*)) );
  }

  if ( directMsgParser ) {
    connect( directMsgParser, SIGNAL(dataParsed(QString)), this, SIGNAL(dataParsed(QString)));
    connect( directMsgParser, SIGNAL(newEntry(Entry*)), coreParent, SLOT(newEntry(Entry*)) );
  }

  connect( this, SIGNAL(authenticationRequired(QString,quint16,QAuthenticator*)), this, SLOT(slotAuthenticationRequired(QString,quint16,QAuthenticator*)));
  connect( this, SIGNAL(cookieReceived(QStringList)), coreParent, SLOT(storeCookie(QStringList)) );
}

void XmlDownload::extractId( Entry *entry )
{
  emit deleteEntry( entry->id() );
}

XmlData* XmlDownload::processedRequest( ContentRequested content )
{
  switch ( content ) {
    case DirectMessages:
      return &directMessagesData;
      break;
    case Statuses:
    default:
      return &statusesData;
  }
}

XmlData* XmlDownload::processedRequest( int requestId )
{
  if ( requestId == directMessagesData.id ) {
    return &directMessagesData;
  }
  return &statusesData;
}

void XmlDownload::getContent( const QString &path, ContentRequested content )
{
  QByteArray encodedPath = prepareRequest( path );
  if ( encodedPath == "invalid" ) {
    httpRequestAborted = true;
    return;
  }
  httpGetId = get( encodedPath, buffer );
  processedRequest( content )->assign( httpGetId, buffer, bytearray );
  qDebug() << "Request of type GET and id" << httpGetId << "started";
}

void XmlDownload::postContent( const QString &path, const QByteArray &status, ContentRequested content )
{
  QByteArray encodedPath = prepareRequest( path );
  if ( encodedPath == "invalid" ) {
    httpRequestAborted = true;
    return;
  }
  httpGetId = post( encodedPath, status, buffer );
  processedRequest( content )->assign( httpGetId, buffer, bytearray );
  qDebug() << "Request of type POST and id" << httpGetId << "started";
}

void XmlDownload::readResponseHeader(const QHttpResponseHeader &responseHeader)
{
  //qDebug() << responseHeader.values() ;// allValues( "Set-Cookie" );
  //emit cookieReceived( responseHeader.allValues( "Set-Cookie" ) );
  switch (responseHeader.statusCode()) {
  case 200:                   // Ok
  case 301:                   // Moved Permanently
  case 302:                   // Found
  case 303:                   // See Other
  case 307:                   // Temporary Redirect
    // these are not error conditions
    break;
  case 404:                   // Not Found
    if ( connectionRole == Destroy ) {
      QRegExp rx( "/statuses/destroy/(\\d+)\\.xml", Qt::CaseInsensitive );
      rx.indexIn( url.path() );
      emit deleteEntry( rx.cap(1).toInt() );
    }
  default:
    //emit errorMessage( "Download failed: " + responseHeader.reasonPhrase() );
    httpRequestAborted = true;
    abort();
    processedRequest( currentId() )->clear();
  }
}

void XmlDownload::httpRequestFinished(int requestId, bool error)
{
  closeId = close();
  if (httpRequestAborted) {
    processedRequest( requestId )->clear();
    qDebug() << "request aborted";
    authenticated = false;
    return;
  }
  if (requestId != statusesData.id && requestId != directMessagesData.id )
    return;
  
  buffer->close();

  if (error) {
    emit errorMessage( "Download failed: " + errorString() );
  } else {
    QXmlSimpleReader xmlReader;
    QXmlInputSource source;
    if ( requestId == statusesData.id ) {
      qDebug() << "parsing statuses data";
      source.setData( *statusesData.bytearray );
      xmlReader.setContentHandler( statusParser );
    } else if ( requestId == directMessagesData.id ) {
      qDebug() << "parsing direct messages data";
      source.setData( *directMessagesData.bytearray );
      xmlReader.setContentHandler( directMsgParser );
    }
    xmlReader.parse( source );
    if ( requestId == statusesData.id ) {
      emit finished( Statuses );
    } else if ( requestId == directMessagesData.id ) {
      emit finished( DirectMessages );
    }
    qDebug() << "========= XML PARSING FINISHED =========";
  }
  processedRequest( requestId )->clear();
  authenticated = false;
}

void XmlDownload::slotAuthenticationRequired(const QString & /* hostName */, quint16, QAuthenticator *authenticator)
{
  qDebug() << "auth required";
  if ( authenticated ) {
    qDebug() << "auth dialog";
    switch ( core->authDataDialog() ) {
      case Core::Rejected:
        emit errorMessage( tr("Authentication is required to post updates.") );
      case Core::SwitchToPublic:
        httpRequestAborted = true;
        authenticated = false;
        abort();
        return;
      default:
        break;
    }
//    if ( core->authDataDialog() == Rejected ) {
//      emit errorMessage( tr("Authentication is required to post updates.") );
//      httpRequestAborted = true;
//      abort();
//      return;
//    }
  }
  *authenticator = core->getAuthData();
  authenticated = true;
}
