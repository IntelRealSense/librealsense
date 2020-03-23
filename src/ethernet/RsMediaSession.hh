/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// A data structure that represents a session that consists of
// potentially multiple (audio and/or video) sub-sessions
// (This data structure is used for media *receivers* - i.e., clients.
//  For media streamers, use "ServerMediaSession" instead.)
// C++ header

#pragma once

#include "MediaSession.hh"

class RsMediaSubsession; // forward

class RsMediaSession : public MediaSession
{
public:
  static RsMediaSession *createNew(UsageEnvironment &env,
                                   char const *sdpDescription);

protected:
  RsMediaSession(UsageEnvironment &env);
  // called only by createNew();
  virtual ~RsMediaSession();

  virtual MediaSubsession *createNewMediaSubsession();

  friend class RsMediaSubsessionIterator;
};

class RsMediaSubsessionIterator
{
public:
  RsMediaSubsessionIterator(RsMediaSession const &session);
  virtual ~RsMediaSubsessionIterator();

  RsMediaSubsession *next(); // NULL if none
  void reset();

private:
  RsMediaSession const &fOurSession;
  RsMediaSubsession *fNextPtr;
};

class RsMediaSubsession : public MediaSubsession
{
protected:
  friend class RsMediaSession;
  friend class RsMediaSubsessionIterator;
  RsMediaSubsession(RsMediaSession &parent);
  virtual ~RsMediaSubsession();
  virtual Boolean createSourceObjects(int useSpecialRTPoffset);
  // create "fRTPSource" and "fReadSource" member objects, after we've been initialized via SDP
};

