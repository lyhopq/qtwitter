/***************************************************************************
 *   Copyright (C) 2008-2009 by Dominik Kapusta       <d@ayoy.net>         *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of      *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to                     *
 *   the Free Software Foundation, Inc.,                                   *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#ifndef TWITTERAPI_P_H
#define TWITTERAPI_P_H

#include <QObject>

#include "twitterapi.h"

class TwitterAPIPrivate : public QObject
{
  Q_OBJECT
public:

  ~TwitterAPIPrivate();
  void createInterface();

  void parseXml( const QByteArray &data, XmlParser *parser );
  void emitUnauthorized( QNetworkReply *reply );
  QByteArray prepareRequest( const QString &data, quint64 inReplyTo );
  QByteArray prepareRequest( const QString &screenName, const QString & );

  void init();
  QByteArray prepareOAuthString( const QString &requestUrl, QOAuth::HttpMethod method,
                                 const QOAuth::ParamMap &params = QOAuth::ParamMap() );

  bool usingOAuth;
  QString login;
  QString password;
  QString serviceUrl;

  Interface *interface;

  QXmlSimpleReader *xmlReader;
  QXmlInputSource *source;

  QOAuth::Interface *qoauth;

  static const QNetworkRequest::Attribute ATTR_SOCIALNETWORK;
  static const QNetworkRequest::Attribute ATTR_ROLE;
  static const QNetworkRequest::Attribute ATTR_LOGIN;
  static const QNetworkRequest::Attribute ATTR_PASSWORD;
  static const QNetworkRequest::Attribute ATTR_STATUS;
  static const QNetworkRequest::Attribute ATTR_ID;
  static const QNetworkRequest::Attribute ATTR_DM_REQUESTED;
  static const QNetworkRequest::Attribute ATTR_DM_RECIPIENT;
  static const QNetworkRequest::Attribute ATTR_DELETION_REQUESTED;
  static const QNetworkRequest::Attribute ATTR_MSGCOUNT;

  static const QString UrlStatusesPublicTimeline;
  static const QString UrlStatusesFriendsTimeline;
  static const QString UrlStatusesUpdate;
  static const QString UrlStatusesDestroy;
  static const QString UrlDirectMessages;
  static const QString UrlDirectMessagesNew;
  static const QString UrlDirectMessagesDestroy;
  static const QString UrlFavoritesCreate;
  static const QString UrlFavoritesDestroy;
  static const QString UrlFriendshipCreate;
  static const QString UrlFriendshipDestroy;

public slots:
  void requestFinished( QNetworkReply *reply );
  void slotAuthenticationRequired( QNetworkReply *reply, QAuthenticator *authenticator );

protected:
  TwitterAPI *q_ptr;

private:
  Q_DECLARE_PUBLIC(TwitterAPI);
};

#endif // TWITTERAPI_P_H
