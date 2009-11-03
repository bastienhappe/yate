/**
 * yatejabber.h
 * Yet Another Jabber Stack
 * This file is part of the YATE Project http://YATE.null.ro
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2004-2006 Null Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __YATEJABBER_H
#define __YATEJABBER_H

#include <xmpputils.h>

/**
 * Holds all Telephony Engine related classes.
 */
namespace TelEngine {

class JBEvent;                           // A Jabber event
class JBStream;                          // A Jabber stream
class JBClientStream;                    // A client to server stream
class JBServerStream;                    // A server to server stream
class JBRemoteDomainDef;                 // Options and connect settings for a remote domain
class JBConnect;                         // A socket connector
class JBEngine;                          // A Jabber engine
class JBServerEngine;                    // A Jabber server engine
class JBClientEngine;                    // A Jabber client engine
class JBStreamSet;                       // A set of streams to be processed in an uniform way
class JBStreamSetProcessor;              // Specialized stream processor
class JBStreamSetReceive;                // Specialized stream data receiver
class JBStreamSetList;                   // A list of stream sets
class JBEntityCaps;                      // Entity capability
class JBEntityCapsList;                  // Entity capability list manager


/**
 * Default port for client to server connections
 */
#define XMPP_C2S_PORT 5222

/**
 * Default port for server to server connections
 */
#define XMPP_S2S_PORT 5269

/**
 * Default value for maximum length of an incomplete xml allowed in a stream
 * parser's buffer
 */
#define XMPP_MAX_INCOMPLETEXML 8192


/**
 * This class handles PLAIN (rfc 4616) and DIGEST (rfc 2831) SASL authentication
 * @short SASL authentication mechanism
 */
class YJABBER_API SASL : public GenObject
{
    YCLASS(SASL,GenObject)
public:
    /**
     * Constructor
     * @param plain True to build a plain password auth object
     * @param realm Optional server realm
     */
    SASL(bool plain, const char* realm = 0);

    /**
     * Destructor
     */
    ~SASL()
	{ TelEngine::destruct(m_params); }

    /**
     * Set auth params
     * @param user Optional username
     * @param pwd Optional password
     */
    void setAuthParams(const char* user = 0, const char* pwd = 0);

    /**
     * Build a client initial auth or challenge response
     * @param buf Destination buffer. It will be filled with Base64 encoded result
     * @param digestUri Digest MD5 URI
     * @return True on success
     */
    bool buildAuthRsp(String& buf, const char* digestUri = 0);

    /**
     * Build a server reply to challenge response
     * @param buf Destination buffer. It will be filled with Base64 encoded result
     * @param rsp The response
     */
    inline void buildAuthRspReply(String& buf, const String& rsp) {
	    if (m_plain)
		return;
	    String tmp("rspauth=" + rsp);
	    Base64 b((void*)tmp.c_str(),tmp.length(),false);
	    b.encode(buf);
	    b.clear(false);
	}

    /**
     * Check if a challenge response reply is valid
     * @param reply The reply to check
     * @return True if valid
     */
    inline bool validAuthReply(const String& reply) {
	    String tmp;
	    if (m_params)
		buildMD5Digest(tmp,m_params->getValue("password"),false);
	    return tmp == reply;
	}

    /**
     * Build an MD5 challenge from this object.
     * Generate a new nonce and increase nonce count
     * @param buf Destination buffer
     * @return True on success
     */
    bool buildMD5Challenge(String& buf);

    /**
     * Build a Digest MD5 SASL (RFC 2831) to be sent with authentication responses
     * @param dest Destination string
     * @param password The password to use
     * @param challengeRsp True if building a Digest MD5 challenge response, false if
     *  building a challenge response reply
     */
    inline void buildMD5Digest(String& dest, const char* password,
	bool challengeRsp = true) {
	    if (m_params)
		buildMD5Digest(dest,*m_params,password,challengeRsp);
	}

    /**
     * Parse plain password auth data
     * @param buf The buffer to parse
     * @return True if succesfully parsed
     */
    bool parsePlain(const DataBlock& buf);

    /**
     * Parse and decode a buffer containing a SASL Digest MD5 challenge.
     * @param buf Already checked for valid UTF8 characters input string
     * @return True on success
     */
    bool parseMD5Challenge(const String& buf);

    /**
     * Parse and decode a buffer containing a SASL Digest MD5 response.
     * Check realm, nonce and nonce count
     * @param buf Already checked for valid UTF8 characters input string
     * @return True on success
     */
    bool parseMD5ChallengeRsp(const String& buf);

    /**
     * Parse and decode a buffer containing SASL plain authentication data
     *  as defined in RFC 4616
     * @param buf Input buffer
     * @param user Destination buffer for username part
     * @param pwd Destination buffer for password part
     * @param authzid Optional destination buffer for authorization identity part
     * @return True on success
     */
    static bool parsePlain(const DataBlock& buf, String& user, String& pwd,
	String* authzid = 0);

    /**
     * Build a Digest MD5 SASL (RFC 2831) to be sent with authentication responses
     * @param dest Destination string
     * @param params List of parameters
     * @param password The password to use
     * @param challengeRsp True if building a Digest MD5 challenge response, false if
     *  building a challenge response reply
     */
    static void buildMD5Digest(String& dest, const NamedList& params,
	const char* password, bool challengeRsp = true);

    bool m_plain;
    NamedList* m_params;
    String m_realm;
    String m_nonce;
    String m_cnonce;
    unsigned int m_nonceCount;

private:
    SASL() {}
};


/**
 * This class holds a Jabber stream event. Stream events are raised by streams
 *  and sent by the engine to the proper service
 * @short A Jabber stream event
 */
class YJABBER_API JBEvent : public RefObject
{
    YCLASS(JBEvent,RefObject)
    friend class JBStream;
    friend class JBClientStream;
    friend class JBServerStream;
public:
    /**
     * Event type enumeration
     */
    enum Type {
	// Stream terminated. Try to connect or wait to be destroyed
	Terminated,
	// Stream is destroying
	Destroy,
	// Stream start was received: when processing this event, the upper
	// layer must call stream's start() method or terminate the stream
	Start,
	// Incoming stream need auth: when processing this event, the upper
	// layer must call stream's authenticated() method
	Auth,
	// The event's element is an 'iq' with a child qualified by bind namespace
	// This event is generated by an incoming client stream without a bound resource
	Bind,
	// Stream is running (can send/recv stanzas)
	Running,
	// The event's element is a 'message'
	Message,
	// The event's element is a 'presence'
	Presence,
	// The event's element is an 'iq'
	Iq,
	// The event's element is a 'db:result' one received by a server-to-server stream
	//  containing the dialback key to verify
	// The event's text is filled with dialback key to verify
	DbResult,
	// The event's element is a 'db:verify' one received by a server-to-server stream
	DbVerify,
	// New user register or user password change succeeded
	RegisterOk,
	// New user register or user password change failed
	// The event's element is the response
	RegisterFailed,
	// Non stanza element received in Running state
	Unknown
    };

    /**
     * Constructor. Constructs an event from a stream
     * @param type Type of this event
     * @param stream The stream that generated the event
     * @param element Element that generated the event
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @param child Optional type depending element's child
     */
    inline JBEvent(Type type, JBStream* stream, XmlElement* element,
	const JabberID& from, const JabberID& to, XmlElement* child = 0)
	: m_type(type), m_stream(0), m_link(true), m_element(element),
	m_child(child)
	{ init(stream,element,&from,&to); }

    /**
     * Constructor. Constructs an event from a stream
     * @param type Type of this event
     * @param stream The stream that generated the event
     * @param element Element that generated the event
     * @param child Optional type depending element's child
     */
    inline JBEvent(Type type, JBStream* stream, XmlElement* element,
	XmlElement* child = 0)
	: m_type(type), m_stream(0), m_link(true), m_element(element),
	m_child(child)
	{ init(stream,element); }

    /**
     * Destructor. Delete the XML element if valid
     */
    virtual ~JBEvent();

    /**
     * Get the event type
     * @return The type of this event as enumeration
     */
    inline int type() const
	{ return m_type; }

    /**
     * Get the event name
     * @return The name of this event
     */
    inline const char* name() const
	{ return lookup(type()); }

    /**
     * Get the element's 'type' attribute if any
     * @return The  element's 'type' attribute
     */
    inline const String& stanzaType() const
	{ return m_stanzaType; }

    /**
     * Get the 'from' attribute of a received stanza
     * @return The 'from' attribute
     */
    inline const JabberID& from() const
	{ return m_from; }

    /**
     * Get the 'to' attribute of a received stanza
     * @return The 'to' attribute
     */
    inline const JabberID& to() const
	{ return m_to; }

    /**
     * Get the sender's id for Write... events or the 'id' attribute if the
     *  event carries a received stanza
     * @return The event id
     */
    inline const String& id() const
	{ return m_id; }

    /**
     * The stanza's text or termination reason for Terminated/Destroy events
     * @return The event's text
     */
    inline const String& text() const
	{ return m_text; }

    /**
     * Get the stream that generated this event
     * @return The stream that generated this event
     */
    inline JBStream* stream() const
	{ return m_stream; }

    /**
     * Get a client-to-server stream from the event's stream
     * @return JBClientStream pointer or 0
     */
    JBClientStream* clientStream();

    /**
     * Get a server-to-server stream from the event's stream
     * @return JBServerStream pointer or 0
     */
    JBServerStream* serverStream();

    /**
     * Get the underlying XmlElement
     * @return XmlElement pointer or 0
     */
    inline XmlElement* element() const
	{ return m_element; }

    /**
     * Get the first child of the underlying element if any
     * @return XmlElement pointer or 0
     */
    inline XmlElement* child() const
	{ return m_child; }

    /**
     * Delete the underlying XmlElement(s). Release the ownership.
     * The caller will own the returned pointer
     * @param del True to delete all xml elements owned by this event
     * @return XmlElement pointer if not deleted or 0
     */
    XmlElement* releaseXml(bool del = false);

    /**
     * Build an 'iq' result stanza from event data
     * @param addTags True to add the 'from' and 'to' attributes
     * @param child Optional 'iq' child (will be consumed)
     * @return True on success
     */
    XmlElement* buildIqResult(bool addTags, XmlElement* child = 0);

    /**
     * Build and send a stanza 'result' from enclosed 'iq' element
     * Release the element on success
     * @param child Optional 'iq' child (will be consumed)
     * @return True on success
     */
    bool sendIqResult(XmlElement* child = 0);

    /**
     * Build an 'iq' error stanza from event data
     * The event's element will be released and added to the error one
     *  if the id is empty
     * @param addTags True to add the 'from' and 'to' attributes
     * @param error Error to be returned to the event's XML sender
     * @param reason Optional text to be attached to the error
     * @param type Error type
     * @return True on success
     */
    XmlElement* buildIqError(bool addTags, XMPPError::Type error, const char* reason = 0,
	XMPPError::ErrorType type = XMPPError::TypeModify);

    /**
     * Build and send a stanza error from enclosed element
     * Release the element on success
     * @param error Error to be returned to the event's XML sender
     * @param reason Optional text to be attached to the error
     * @param type Error type
     * @return True on success
     */
    bool sendStanzaError(XMPPError::Type error, const char* reason = 0,
	XMPPError::ErrorType type = XMPPError::TypeModify);

    /**
     * Release the link with the stream to let the stream continue with events
     * @param release True to release the reference to the stream
     */
    void releaseStream(bool release = false);

    /**
     * Get the name of an event type
     * @return The name an event type
     */
    inline static const char* lookup(int type)
	{ return TelEngine::lookup(type,s_type); }

private:
    static const TokenDict s_type[];     // Event names
    JBEvent() {}                         // Don't use it!
    bool init(JBStream* stream, XmlElement* element,
	const JabberID* from = 0, const JabberID* to = 0);

    Type m_type;                         // Type of this event
    JBStream* m_stream;                  // The stream that generated this event
    bool m_link;                         // Stream link state
    XmlElement* m_element;               // Received XML element, if any
    XmlElement* m_child;                 // The first child element for 'iq' elements
    String m_stanzaType;                 // Stanza's 'type' attribute
    JabberID m_from;                     // Stanza's 'from' attribute
    JabberID m_to;                       // Stanza's 'to' attribute
    String m_id;                         // 'id' attribute if the received element has one
    String m_text;                       // The stanza's text or termination reason for
                                         //  Terminated/Destroy events
};


/**
 * Base class for all Jabber streams. Basic stream data processing: send/receive
 *  XML elements, keep stream state, generate events
 * @short A Jabber stream
 */
class YJABBER_API JBStream : public RefObject, public DebugEnabler, public Mutex
{
    friend class JBEngine;
    friend class JBEvent;
public:
    /**
     * Stream type enumeration
     */
    enum Type {
	c2s       = 0,                   // Client to server
	s2s       = 1,                   // Server to server
	TypeCount = 2                    // Unknown
    };

    /**
     * Stream state enumeration
     */
    enum State {
	Idle             = 0,            // Stream is waiting to be connected or destroyed
	Connecting       = 1,            // Outgoing stream is waiting for the socket to connect
	WaitStart        = 2,            // Waiting for remote's stream start
	                                 // (outgoing: stream start already sent)
	Starting         = 3,            // Incoming stream is processing a stream start element
	Features         = 4,            // Outgoing: waiting for stream features
	                                 // Incoming: stream features sent
	WaitTlsRsp       = 5,            // 'starttls' sent: waiting for response
	Securing         = 10,           // Stream is currently negotiating the TLS
	Auth             = 11,           // Auth element (db:result for s2s streams) sent
	Challenge        = 12,           // 'challenge' element sent/received
	Register         = 20,           // A new user is currently registering
	Running          = 100,          // Established. Allow XML stanzas to pass over the stream
	Destroy,                         // Stream is destroying. No more traffic allowed
    };

    /**
     * Stream behaviour options
     */
    enum Flags {
	NoAutoRestart       = 0x00000001,// Don't restart stream when down
	TlsRequired         = 0x00000002,// TLS is mandatory on this stream
	AllowPlainAuth      = 0x00000004,// Allow plain password authentication
	                                 //  If not allowed and this is the only method
	                                 //  offered by server the stream will be terminated
	DialbackOnly        = 0x00000008,// Outgoing s2s dialback stream
	RegisterUser        = 0x00000010,// Outgoing c2s register new user
	// Flags to be managed by the upper layer
	RosterRequested     = 0x00000100,// c2s: the roster was already requested
	AvailableResource   = 0x00000200,// c2s: available presence was sent/received
	PositivePriority    = 0x00000400,// c2s: the resource advertised by the client has priority >= 0
	// Internal flags (cleared when the stream is re-started)
	StreamSecured       = 0x00020000,// TLS stage was done (possible without using TLS)
	StreamTls           = 0x00040000,// The stream is using TLS
	StreamAuthenticated = 0x00080000,// Stream already authenticated
	StreamRemoteVer1    = 0x00100000,// Remote party advertised RFC3920 version=1.0
	StreamWaitBindRsp   = 0x01000000,// Outgoing c2s waiting for bind response
	StreamWaitSessRsp   = 0x02000000,// Outgoing c2s waiting for session response
	StreamWaitChallenge = 0x04000000,// Outgoing waiting for auth challenge
	StreamWaitChgRsp    = 0x08000000,// Outgoing waiting challenge response confirmation
	StreamRfc3920Chg    = 0x10000000,// Outgoing sent empty response to challenge with rspauth (RFC3920)
	// Flag masks
	StreamFlags         = 0x000000ff,
	InternalFlags       = 0xffff0000,
    };

    /**
     * Destructor.
     * Gracefully close the stream and the socket
     */
    virtual ~JBStream();

    /**
     * Get the type of this stream. See the protocol enumeration of the engine
     * @return The type of this stream
     */
    inline int type() const
	{ return m_type; }

    /**
     * Retrieve this stream's default namespace
     * @return The stream default namespace
     */
    inline int xmlns() const
	{ return m_xmlns; }

    /**
     * Get the stream state
     * @return The stream state as enumeration.
     */
    inline State state() const
	{ return m_state; }

    /**
     * Get the stream direction
     * @return True if the stream is an incoming one
     */
    inline bool incoming() const
	{ return m_incoming; }

    /**
     * Get the stream direction
     * @return True if the stream is an outgoing one
     */
    inline bool outgoing() const
	{ return !m_incoming; }

    /**
     * Get the stream's owner
     * @return Pointer to the engine owning this stream
     */
    inline JBEngine* engine() const
	{ return m_engine; }

    /**
     * Get the stream's name
     * @return The stream's name
     */
    inline const char* name() const
	{ return m_name; }

    /**
     * Get the stream id
     * @return The stream id
     */
    inline const String& id() const
	{ return m_id; }

    /**
     * Get the JID of the local side of this stream
     * @return The JID of the local side of this stream
     */
    inline const JabberID& local() const
	{ return m_local; }

    /**
     * Set the local party's JID
     * @param jid Local party's jid to set
     */
    inline void setLocal(const char* jid)
	{ m_local.set(jid); }

    /**
     * Get the JID of the remote side of this stream
     * @return The JID of the remote side of this stream
     */
    inline const JabberID& remote() const
	{ return m_remote; }

    /**
     * Get the remote party's address
     * This method is thread safe
     * @param addr The socket address to be filled with remote party's address
     * @return True on success
     */
    inline bool remoteAddr(SocketAddr& addr) {
	    Lock lock(this);
	    return m_socket && m_socket->getPeerName(addr);
	}

    /**
     * Get the local address
     * This method is thread safe
     * @param addr The socket address to be filled with local address
     * @return True on success
     */
    inline bool localAddr(SocketAddr& addr) {
	    Lock lock(this);
	    return m_socket && m_socket->getSockName(addr);
	}

    /**
     * Get the stream flags
     * @return Stream flags
     */
    inline int flags() const
	{ return m_flags; }

    /**
     * Check if a given option (or option mask) is set
     * @param mask The flag(s) to check
     * @return True if set
     */
    inline bool flag(int mask) const
	{ return 0 != (m_flags & mask); }

    /**
     * Set or reset the TLS required flag
     * This method is not thread safe
     * @param set True to set, false to reset the flag
     */
    inline void setTlsRequired(bool set) {
	    if (set)
		m_flags |= TlsRequired;
	    else
		m_flags &= ~TlsRequired;
	}

    /**
     * Retrieve remote ip/port used to connect to
     * This method is not thread safe
     * @param addr The ip
     * @param port The port
     */
    inline void connectAddr(String& addr, int& port) const {
	    addr = m_connectAddr;
	    port = m_connectPort;
	}

    /**
     * Set/reset RosterRequested flag
     * This method is thread safe
     * @param ok True to set, false to reset it
     */
    void setRosterRequested(bool ok);

    /**
     * Set/reset AvailableResource/PositivePriority flags
     * This method is thread safe
     * @param ok True to set, false to reset it
     * @param positive True if an available resource has positive priority
     * @return True if changed
     */
    bool setAvailableResource(bool ok, bool positive = true);

    /**
     * Read data from socket. Send it to the parser.
     * Terminate the stream on socket or parser error
     * @param buf Destination buffer
     * @param len Buffer length (must be greater then 1)
     * @return True if data was received
     */
    bool readSocket(char* buf, unsigned int len);

    /**
     * Get a client stream from this one
     * @return JBClientStream pointer or 0
     */
    virtual JBClientStream* clientStream()
	{ return 0; }

    /**
     * Get a server stream from this one
     * @return JBServerStream pointer or 0
     */
    virtual JBServerStream* serverStream()
	{ return 0; }

    /**
     * Stream state processor.
     * This method is thread safe
     * @param time Current time
     * @return JBEvent pointer or 0
     */
    JBEvent* getEvent(u_int64_t time = Time::msecNow());

    /**
     * Send a stanza ('iq', 'message' or 'presence') in Running state.
     * This method is thread safe
     * @param xml Element to send (will be consumed and zeroed)
     * @return True on success
     */
    bool sendStanza(XmlElement*& xml);

    /**
     * Send stream related XML when negotiating the stream or some other
     *  stanza in non Running state
     * All elements will be consumed
     * This method is thread safe
     * @param newState The new stream state to set on success
     * @param first The first element to send
     * @param second Optional second element to send
     * @param third Optional third element to send
     * @return True on success
     */
    bool sendStreamXml(State newState, XmlElement* first, XmlElement* second = 0,
	XmlElement* third = 0);

    /**
     * Start the stream. This method should be called by the upper layer
     *  when processing an incoming stream Start event. For outgoing streams
     *  this method is called internally on succesfully connect.
     * This method is thread safe
     * @param features Optional features to advertise to the remote party of an
     *  incoming stream. The caller is responsable of freeing it.
     *  If processed, list's elements will be moved to stream's features list
     * @param caps Optional entity capabilities to be added to the stream features.
     *  Ignored for outgoing streams
     */
    void start(XMPPFeatureList* features = 0, XmlElement* caps = 0);

    /**
     * Auth event result. This method should be called by the
     *  upper layer when processing an Auth event
     * This method is thread safe
     * @param ok True if the remote party was authenticated,
     *  false if authentication failed
     * @param rsp Optional success response content. Ignored if not authenticated
     * @param error Failure reason. Ignored if authenticated
     * @return False if stream state is incorrect
     */
    bool authenticated(bool ok, const String& rsp = String::empty(),
	XMPPError::Type error = XMPPError::NotAuthorized);

    /**
     * Terminate the stream. Send stream end tag or error.
     * Reset the stream. Deref stream if destroying.
     * This method is thread safe
     * @param location The terminate request location:
     *  -1: upper layer, 0: internal, 1: remote
     * @param destroy True to destroy. False to terminate
     * @param xml Received XML element. The element will be consumed
     * @param error Termination reason. Set it to NoError to send stream end tag
     * @param reason Optional text to be added to the error stanza
     * @param final True if called from destructor
     */
    void terminate(int location, bool destroy, XmlElement* xml,
	int error = XMPPError::NoError, const char* reason = "",
	bool final = false);

    /**
     * Outgoing stream connect terminated notification.
     * Send stream start if everithing is ok
     * @param sock The connected socket, will be consumed and zeroed
     */
    virtual void connectTerminated(Socket*& sock);

    /**
     * Get an object from this stream
     * @param name The name of the object to get
     */
    virtual void* getObject(const String& name) const;

    /**
     * Get the name of a stream state
     * @return The name of the stream state
     */
    inline const char* stateName()
	{ return lookup(state(),s_stateName); }

    /**
     * Get the name of a stream type
     * @return The name of the stream type
     */
    inline const char* typeName()
	{ return lookup(type(),s_typeName); }

    /**
     * Get the string representation of this stream
     * @return Stream name
     */
    virtual const String& toString() const;

    /**
     * Get the stream type associated with a given text
     * @param text Stream type text to find
     * @param defVal Value to return if not found
     * @return The stream type associated with a given text
     */
    static inline Type lookupType(const char* text, Type defVal = TypeCount)
	{ return (Type)lookup(text,s_typeName,defVal); }

    /**
     * SASL authentication data
     */
    SASL* m_sasl;

    /**
     * Dictionary keeping the stream state names
     */
    static const TokenDict s_stateName[];

    /**
     * Dictionary keeping the flag names
     */
    static const TokenDict s_flagName[];

    /**
     * Dictionary keeping the stream type names
     */
    static const TokenDict s_typeName[];

protected:
    /**
     * Constructor. Build an incoming stream from a socket
     * @param engine Engine owning this stream
     * @param socket The socket
     * @param t Stream type as enumeration
     */
    JBStream(JBEngine* engine, Socket* socket, Type t);

    /**
     * Constructor. Build an outgoing stream
     * @param engine Engine owning this stream
     * @param t Stream type as enumeration
     * @param local Local party jabber id
     * @param remote Remote party jabber id
     * @param name Optional stream name
     * @param params Optional stream parameters
     */
    JBStream(JBEngine* engine, Type t, const JabberID& local, const JabberID& remote,
	const char* name = 0, const NamedList* params = 0);

    /**
     * Close the stream. Release memory
     */
    virtual void destroyed();

    /**
     * Check if stream state processor can continue.
     * This method is called from getEvent() with the stream locked
     * @param time Current time
     * @return True to indicate stream availability to process its state,
     *  false to return the last event, if any
     */
    virtual bool canProcess(u_int64_t time);

    /**
     * Process stream state. Get XML from parser's queue and process it
     * This method is called from getEvent() with the stream locked
     * @param time Current time
     */
    virtual void process(u_int64_t time);

    /**
     * Process elements in Running state
     * @param xml Received element (will be consumed)
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @return False if stream termination was initiated
     */
    virtual bool processRunning(XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Check stream timeouts.
     * This method is called from getEvent() with the stream locked, after
     *  the process() method returned without setting the last event
     * @param time Current time
     */
    virtual void checkTimeouts(u_int64_t time);

    /**
     * Reset the stream's connection. Build a new XML parser if the socket is valid
     * Release the old connection
     * @param sock The new socket
     */
    virtual void resetConnection(Socket* sock = 0);

    /**
     * Build a stream start XML element
     * @return XmlElement pointer
     */
    virtual XmlElement* buildStreamStart();

    /**
     * Process stream start elements while waiting for them
     * @param xml Received xml element
     * @param from The 'from' attribute
     * @param to The 'to' attribute
     * @return False if stream termination was initiated
     */
    virtual bool processStart(const XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Process elements in Auth state
     * @param xml Received element (will be consumed)
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @return False if stream termination was initiated
     */
    virtual bool processAuth(XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Process elements in Register state
     * @param xml Received element (will be consumed)
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @return False if stream termination was initiated
     */
    virtual bool processRegister(XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Check if a received stream start element is correct.
     * Check namespaces and set stream version
     * Check and set the id for outgoing streams
     * Generate an id for incoming streams
     * Terminate the stream if this conditions are met
     * @param xml Received xml element. Will be consumed and zeroed if false is returned
     * @return False if stream termination was initiated
     */
    bool processStreamStart(const XmlElement* xml);

    /**
     * Check if a received element is a stream error one
     * @param xml Received xml element
     * @return True if stream termination was initiated (the xml will be consumed)
     */
    bool streamError(XmlElement* xml);

    /**
     * Retrieve and check the 'from' and 'to' JIDs from a receive element
     * @param xml Received xml element
     * @param from Jabber ID to set from the 'from' attribute
     * @param to Jabber ID to set from the 'to' attribute
     * @return False if stream termination was initiated (the xml will be consumed)
     */
    bool getJids(XmlElement* xml, JabberID& from, JabberID& to);

    /**
     * Check if a received element is a presence, message or iq qualified by the stream
     *  namespace and the stream is not authenticated.
     * Validate 'from' for c2s streams
     * Validate s2s 'to' domain and 'from' jid
     * Fix 'from' or 'to' is needed
     * @param xml Received xml element (will be consumed if false is returned)
     * @param from The sender of the stanza
     * @param to Stanza recipient
     * @return False if the element was consumed (error was sent or stream termination was initiated)
     */
    bool checkStanzaRecv(XmlElement* xml, JabberID& from, JabberID& to);

    /**
     * Change stream state. Reset state depending data
     * @param newState The new stream state
     * @param time Current time
     */
    void changeState(State newState, u_int64_t time = Time::msecNow());

    /**
     * Check for pending events. Set the last event
     */
    void checkPendingEvent();

    /**
     * Send pending stream XML or stanzas
     * Terminate the stream on error
     * @param streamOnly Try to send only existing stream related XML elements
     * @return True on success
     */
    bool sendPending(bool streamOnly = false);

    /**
     * Write data to socket. Terminate the stream on socket error
     * @param data Buffer to sent
     * @param len The number of bytes to send. Filled with actually sent bytes on exit
     * @return True on success, false if stream termination was initiated
     */
    bool writeSocket(const char* data, unsigned int& len);

    /**
     * Update stream flags and remote connection data from engine
     */
    void updateFromRemoteDef();

    /**
     * Retrieve the first required feature in the list
     * @return XMPPFeature pointer or 0
     */
    XMPPFeature* firstRequiredFeature();

    /**
     * Drop (delete) received XML element
     * @param xml The element to delete
     * @param reason The reason
     * @return True
     */
    bool dropXml(XmlElement*& xml, const char* reason);

    /**
     * Terminate (destroy) the stream. Drop (delete) received XML element
     * @param xml The element to delete
     * @param error Terminate error
     * @param reason Drop reason
     * @return False
     */
    inline bool destroyDropXml(XmlElement*& xml, XMPPError::Type error, const char* reason) {
	    dropXml(xml,reason);
	    terminate(0,true,0,error);
	    return false;
	}

    State m_state;                       // Stream state
    String m_id;                         // Stream id
    JabberID m_local;                    // Local peer's jid
    JabberID m_remote;                   // Remote peer's jid
    int m_flags;                         // Stream flags
    XMPPNamespace::Type m_xmlns;         // Stream namespace
    XMPPFeatureList m_features;          // Advertised features
    JBEvent* m_lastEvent;                // Last event generated by this stream
    ObjList m_events;                    // Queued events
    ObjList m_pending;                   // Pending outgoing elements
    // Timers
    u_int64_t m_setupTimeout;            // Overall stream setup timeout
    u_int64_t m_startTimeout;            // Incoming: wait stream start period
    u_int64_t m_pingTimeout;             // Sent ping timeout
    u_int64_t m_nextPing;                // Next ping
    u_int64_t m_idleTimeout;             // Stream idle timeout
    u_int64_t m_connectTimeout;          // Stream connect timeout
    //
    unsigned int m_restart;              // Remaining restarts
    u_int64_t m_timeToFillRestart;       // The next time to increase the restart counter

    String m_pingId;

private:
    // Forbidden default constructor
    inline JBStream() {}
    // Process incoming elements in Challenge state
    // The element will be consumed
    // Return false if stream termination was initiated
    bool processChallenge(XmlElement* xml, const JabberID& from,
	const JabberID& to);
    // Process incoming 'auth' elements qualified by SASL namespace
    // The element will be consumed
    // Return false if stream termination was initiated
    bool processSaslAuth(XmlElement* xml, const JabberID& from,
	const JabberID& to);
    // Process received elements in Features state (incoming stream)
    // The element will be consumed
    // Return false if stream termination was initiated
    bool processFeaturesIn(XmlElement* xml, const JabberID& from,
	const JabberID& to);
    // Process received elements in Features state (outgoing stream)
    // The element will be consumed
    // Return false if stream termination was initiated
    bool processFeaturesOut(XmlElement* xml, const JabberID& from,
	const JabberID& to);
    // Process received elements in WaitTlsRsp state (outgoing stream)
    // The element will be consumed
    // Return false if stream termination was initiated
    bool processWaitTlsRsp(XmlElement* xml, const JabberID& from,
	const JabberID& to);
    // Set secured flag. Remove feature from list
    inline void setSecured() {
	    m_flags |= StreamSecured;
	    m_features.remove(XMPPNamespace::Tls);
	}
    // Set stream namespace from type
    void setXmlns();
    // Event termination notification
    // @param event The notifier. Ignored if it's not m_lastEvent
    void eventTerminated(const JBEvent* event);

    enum {
	SocketCanRead = 0x01,
	SocketReading = 0x02,
	SocketWriting = 0x10,
    };
    inline void socketSetCanRead(bool ok) {
	    if (ok)
		m_socketFlags |= SocketCanRead;
	    else
		m_socketFlags &= ~SocketCanRead;
	}
    inline void socketSetReading(bool ok) {
	    if (ok)
		m_socketFlags |= SocketReading;
	    else
		m_socketFlags &= ~SocketReading;
	}
    inline void socketSetWriting(bool ok) {
	    if (ok)
		m_socketFlags |= SocketWriting;
	    else
		m_socketFlags &= ~SocketWriting;
	}
    inline bool socketCanRead() const
	{ return m_socket && (m_socketFlags & SocketCanRead); }
    inline bool socketReading() const
	{ return m_socketFlags & SocketReading; }
    inline bool socketWriting() const
	{ return m_socketFlags & SocketWriting; }

    JBEngine* m_engine;                  // The owner of this stream
    int m_type;                          // Stream type
    bool m_incoming;                     // Stream direction
    String m_name;                       // Local (internal) name
    JBEvent* m_terminateEvent;           // Pending terminate event
    // Pending outgoing XML
    String m_outStreamXml;
    // Connection related data
    XmlDomParser* m_xmlDom;
    Socket* m_socket;
    char m_socketFlags;                  // Socket flags: 0: unavailable
    String m_connectAddr;                // Remote ip to connect to
    int m_connectPort;                   // Remote port to connect to
};


/**
 * This class holds a client to server stream
 * @short A client to server stream
 */
class YJABBER_API JBClientStream : public JBStream
{
    YCLASS(JBClientStream,JBStream)
    friend class JBStream;
public:
    /**
     * Constructor. Build an incoming stream from a socket
     * @param engine Engine owning this stream
     * @param socket The socket
     */
    JBClientStream(JBEngine* engine, Socket* socket);

    /**
     * Constructor. Build an outgoing stream
     * @param engine Engine owning this stream
     * @param jid User jid
     * @param account Account (stream) name
     * @param params Stream parameters
     */
    JBClientStream(JBEngine* engine, const JabberID& jid, const String& account,
	const NamedList& params);

    /**
     * Retrieve stream's user data
     * @return GenObject pointer or 0
     */
    inline GenObject* userData()
	{ return m_userData; }

    /**
     * Set stream's user data. Transfer data ownership to the stream
     * This method is thread safe
     * @param data Data to set
     */
    inline void userData(GenObject* data) {
	    Lock lock(this);
	    TelEngine::destruct(m_userData);
	    m_userData = data;
	}

    /**
     * Get a client stream from this one
     * @return JBClientStream pointer
     */
    virtual JBClientStream* clientStream()
	{ return this; }

    /**
     * Bind a resource to an incoming stream. This method should be called
     * after processing a Bind event
     * This method is thread safe
     * @param resource Resource to bind. Empty on error
     * @param id Received bind request id
     * @param error Failure reason. Ignored on success
     */
    void bind(const String& resource, const char* id,
	XMPPError::Type error = XMPPError::NoError);

    /**
     * Request account register or change an outgoing stream.
     * This method is thread safe
     * @param data True to request registration/change, false to request info
     * @param set True to request new user registration, false to remove account from server
     * @param newPass New password when requesting account setup on an already
     *  authenticated stream
     * @return True on success
     */
    bool requestRegister(bool data, bool set = true,
	const String& newPass = String::empty());

protected:
    /**
     * Process elements in Running state
     * @param xml Received element (will be consumed)
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @return False if stream termination was initiated
     */
    virtual bool processRunning(XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Process stream start elements while waiting for them
     * @param xml Received xml element
     * @param from The 'from' attribute
     * @param to The 'to' attribute
     * @return False if stream termination was initiated
     */
    virtual bool processStart(const XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Process elements in Auth state
     * @param xml Received element (will be consumed)
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @return False if stream termination was initiated
     */
    virtual bool processAuth(XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Process elements in Register state
     * @param xml Received element (will be consumed)
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @return False if stream termination was initiated
     */
    virtual bool processRegister(XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Release memory
     */
    virtual void destroyed();

    /**
     * Start outgoing stream authentication
     * @return True on success
     */
    bool startAuth();

    /**
     * Start resource binding on outgoing stream
     * @return True on success
     */
    bool bind();

private:
    inline bool isRegisterId(XmlElement& xml) {
	    if (!m_registerReq)
		return false;
	    String* id = xml.getAttribute("id");
	    return id && id->length() == 1 && (*id)[0] == m_registerReq;
	}

    GenObject* m_userData;               // User (upper layer) data
    String m_password;                   // The password
    String m_newPassword;                // New password
    char m_registerReq;                  // Register requested. 1(data) 2(register) 3(remove)
};


/**
 * This class holds a server to server stream
 * @short A server to server stream
 */
class YJABBER_API JBServerStream : public JBStream
{
    YCLASS(JBServerStream,JBStream)
public:
    /**
     * Constructor. Build an incoming stream from a socket
     * @param engine Engine owning this stream
     * @param socket The socket
     */
    JBServerStream(JBEngine* engine, Socket* socket);

    /**
     * Constructor. Build an outgoing stream
     * @param engine Engine owning this stream
     * @param local Local party jabber id
     * @param remote Remote party jabber id
     * @param dbId Optional dialback id (stream id)
     * @param dbKey Optional dialback key to verify
     * @param dbOnly True if this is a dialback only stream
     */
    JBServerStream(JBEngine* engine, const JabberID& local, const JabberID& remote,
	const char* dbId = 0, const char* dbKey = 0, bool dbOnly = false);

    /**
     * Check if this is an outgoing dialback stream
     * @return True if this stream is an outgoing dialback one
     */
    inline bool dialback() const
	{ return outgoing() && flag(DialbackOnly); }

    /**
     * Take the dialback key from this stream
     * @return NamedString pointer or 0 if there is no dialback key held by this stream
     */
    inline NamedString* takeDb() {
	    Lock lock(this);
	    NamedString* tmp = m_dbKey;
	    m_dbKey = 0;
	    return tmp;
	}

    /**
     * Get a server stream from this one
     * @return JBServerStream pointer
     */
    virtual JBServerStream* serverStream()
	{ return this; }

    /**
     * Send a dialback verify response
     * @param from The 'from' attribute
     * @param to The 'to' attribute
     * @param id The 'id' attribute
     * @param valid True if valid, false if invalid
     * @return True on success
     */
    inline bool sendDbVerify(const char* from, const char* to, const char* id,
	bool valid) {
	    XmlElement* rsp = XMPPUtils::createDialbackVerifyRsp(from,to,id,valid);
	    return sendStreamXml(state(),rsp);
	}

    /**
     * Send a dialback key response.
     * If the stream is in Dialback state change it's state to Running if valid or
     *  terminate it if invalid
     * @param from The 'from' attribute
     * @param to The 'to' attribute
     * @param valid True if valid, false if invalid
     * @return True on success
     */
    bool sendDbResult(const JabberID& from, const JabberID& to, bool valid);

    /**
     * Send dialback data (key/verify)
     * @return Flase if stream termination was initiated
     */
    bool sendDialback();

protected:
    /**
     * Release memory
     */
    virtual void destroyed();

    /**
     * Process elements in Running state
     * @param xml Received element (will be consumed)
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @return False if stream termination was initiated
     */
    virtual bool processRunning(XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Build a stream start XML element
     * @return XmlElement pointer
     */
    virtual XmlElement* buildStreamStart();

    /**
     * Process stream start elements while waiting for them
     * @param xml Received xml element
     * @param from The 'from' attribute
     * @param to The 'to' attribute
     * @return False if stream termination was initiated
     */
    virtual bool processStart(const XmlElement* xml, const JabberID& from,
	const JabberID& to);

    /**
     * Process elements in Auth state
     * @param xml Received element (will be consumed)
     * @param from Already parsed source JID
     * @param to Already parsed destination JID
     * @return False if stream termination was initiated
     */
    virtual bool processAuth(XmlElement* xml, const JabberID& from,
	const JabberID& to);

private:
    NamedString* m_dbKey;                // Outgoing: initial dialback key to check
};


/**
 * This class holds data related to a remote domain.
 * The String holds the domain
 * @short Options and connect settings for a remote domain
 */
class YJABBER_API JBRemoteDomainDef : public String
{
    YCLASS(JBRemoteDomainDef,String)
public:
    /**
     * Constructor
     * @param domain Domain name
     */
    inline JBRemoteDomainDef(const char* domain = 0)
	: String(domain), m_port(0), m_flags(0)
	{}

    /**
     * Remote address used to connect to
     */
    String m_address;

    /**
     * Remote port used to connect to
     */
    int m_port;

    /**
     * Domain flags
     */
    int m_flags;
};


/**
 * This class holds data used to connect an outgoing stream
 * A descendant class should implement the thread run method
 * @short A socket connector
 */
class YJABBER_API JBConnect : public GenObject
{
    YCLASS(JBConnect,GenObject)
public:
    /**
     * Constructor. Add itself to the stream's engine
     * @param stream The stream to connect
     */
    JBConnect(const JBStream& stream);

    /**
     * Destructor. Remove from engine if still there
     */
    virtual ~JBConnect();

    /**
     * Stop the thread. This method should be re-implemented
     */
    virtual void stopConnect();

    /**
     * Retrieve the stream name
     * @return Stream name
     */
    virtual const String& toString() const;

protected:
    /**
     * Connect the socket.
     * Retrieve ip/port from engine ant use them if valid or try to use SRV records returned by
     * the given domain or use the domain's ip address and the default port given by the stream type.
     * Notify the stream on termination.
     * This method should be called from it's own thread
     */
    void connect();

private:
    // No default constructor
    inline JBConnect()
	{}
    // Check if exiting. Release socket if exiting
    bool exiting(Socket*& sock);
    // Connect a socket
    bool connect(Socket*& sock, const char* addr, int port);
    // Notify termination, remove from engine
    void terminated(Socket* sock, bool final);

    String m_domain;                     // Remote domain
    String m_address;                    // Remote ip address
    int m_port;                          // Port to connect to
    JBEngine* m_engine;                  // The engine owning this connector
    String m_stream;                     // Stream name
    JBStream::Type m_streamType;         // Stream type
};


/**
 * This class holds a Jabber engine
 * @short A Jabber engine
 */
class YJABBER_API JBEngine : public DebugEnabler, public Mutex, public GenObject
{
    YCLASS(JBEngine,GenObject)
    friend class JBStream;
    friend class JBConnect;
    friend class JBStreamSetProcessor;
public:
    /**
     * Constructor
     * @param name Engine name
     */
    JBEngine(const char* name = "jbengine");

    /**
     * Destructor
     */
    virtual ~JBEngine();

    /**
     * Retrieve the stream read buffer length
     * @return Stream read buffer length
     */
    inline unsigned int streamReadBuffer() const
	{ return m_streamReadBuffer; }

    /**
     * Check if this engine is exiting
     * @return True if this engine is exiting
     */
    inline bool exiting() const
	{ return m_exiting; }

    /**
     * Set the exiting flag. Terminate all streams
     */
    inline void setExiting() {
	    if (m_exiting)
		return;
	    m_exiting = true;
	    dropAll(JBStream::TypeCount,JabberID::empty(),JabberID::empty(),
		XMPPError::Shutdown);
	}

    /**
     * Find a remote domain definition. Return the default settings if not found.
     * This method is not thread safe
     * @param domain The domain to find
     * @return Valid JBRemoteDomainDef pointer
     */
    inline JBRemoteDomainDef* remoteDomainDef(const String& domain) {
	    ObjList* o = m_remoteDomains.find(domain);
	    return o ? static_cast<JBRemoteDomainDef*>(o->get()) : &m_remoteDomain;
	}

    /**
     * Cleanup streams. Stop all threads owned by this engine. Release memory
     */
    virtual void destruct();

    /**
     * Initialize the engine's parameters. Start private streams if requested
     * @param params Engine's parameters
     */
    virtual void initialize(const NamedList& params);

    /**
     * Stop connect threads. Drop all streams. Stop all stream sets. Release memory if final
     * @param final True if called from destructor
     * @param waitTerminate True to wait for all streams to terminate
     */
    virtual void cleanup(bool final = false, bool waitTerminate = true);

    /**
     * Accept an incoming stream connection. Build a stream.
     * Don't delete the socket if false is returned
     * @param sock Accepted socket
     * @param remote Remote ip and port
     * @param t Expected stream type
     * @return True on success
     */
    bool acceptConn(Socket* sock, SocketAddr& remote, JBStream::Type t);

    /**
     * Find a stream by its name. This method is thread safe
     * @param id The internal id of the stream to find
     * @param hint Optional stream type hint
     * @return Referenced JBStream pointer or 0
     */
    virtual JBStream* findStream(const String& id,
	JBStream::Type hint = JBStream::TypeCount);

    /**
     * Find all c2s streams whose local or remote bare jid matches a given one.
     * Ignore destroying streams.
     * This method is thread safe
     * @param in True for incoming, false for outgoing
     * @param jid JID to compare (the local one for outgoing, remote jid for incoming)
     * @param flags Optional stream flag to match
     * @return List of referenced JBClientStream pointers or 0
     */
    ObjList* findClientStreams(bool in, const JabberID& jid, int flags = 0xffffffff);

    /**
     * Find all c2s streams whose local or remote bare jid matches a given one and
     *  their resource is found in the given list.
     * Ignore destroying streams.
     * This method is thread safe
     * @param in True for incoming, false for outgoing
     * @param jid JID to compare (the local one for outgoing, remote jid for incoming)
     * @param resources The list of resources to match
     * @param flags Optional stream flag to match
     * @return List of referenced JBClientStream pointers or 0
     */
    ObjList* findClientStreams(bool in, const JabberID& jid, const ObjList& resources,
	int flags = 0xffffffff);

    /**
     * Find a c2s stream by its local or remote jid.
     * This method is thread safe
     * @param in True for incoming, false for outgoing
     * @param jid JID to compare (the local one for outgoing, remote jid for incoming)
     * @return Referenced JBClientStream pointer or 0
     */
    JBClientStream* findClientStream(bool in, const JabberID& jid);

    /**
     * Terminate all streams matching type and/or local/remote jid
     * @param type Stream type. Match all stream types if unknown
     * @param local Optional local jid to match
     * @param remote Optional remote jid to match
     * @param error Optional error to be sent to the client
     * @param reason Optional error text to be sent to the client
     * @return The number of stream terminated
     */
    virtual unsigned int dropAll(JBStream::Type type = JBStream::TypeCount,
	const JabberID& local = JabberID::empty(),
	const JabberID& remote = JabberID::empty(),
	XMPPError::Type error = XMPPError::NoError, const char* reason = 0);

    /**
     * Build an internal stream name
     * @param name Destination buffer
     */
    virtual void buildStreamName(String& name)
	{}

    /**
     * Check if a domain is serviced by this engine
     * @param domain Domain to check
     * @return True if the given domain is serviced by this engine
     */
    virtual bool hasDomain(const String& domain)
	{ return false; }

    /**
     * Process an event. The default implementation will return the event
     *  to this engine
     * @param ev The event to process
     */
    virtual void processEvent(JBEvent* ev);

    /**
     * Return an event to this engine. The default implementation will send an
     * error if apropriate and delete the event
     * @param ev The event to return
     * @param error Optional error to be returned to the event's XML sender
     * @param reason Optional text to be attached to the error
     */
    virtual void returnEvent(JBEvent* ev, XMPPError::Type error = XMPPError::NoError,
	const char* reason = 0);

    /**
     * Start stream TLS
     * @param stream The stream to enchrypt
     */
    virtual void encryptStream(JBStream* stream);

    /**
     * Connect an outgoing stream
     * @param stream The stream to connect
     */
    virtual void connectStream(JBStream* stream);

    /**
     * Build a dialback key
     * @param id The stream id
     * @param key The dialback key
     */
    virtual void buildDialbackKey(const String& id, String& key);

    /**
     * Check if an outgoing stream exists with the same id and remote peer
     * @param stream The calling stream
     * @return True if a duplicate is found
     */
    bool checkDupId(const JBStream* stream);

    /**
     * Print XML to output
     * @param stream Stream requesting the operation
     * @param send True if sending, false if receiving
     * @param xml XML to print
     */
    virtual void printXml(const JBStream* stream, bool send, XmlChild& xml) const;

    /**
     * Print an XML fragment to output
     * @param stream Stream requesting the operation
     * @param send True if sending, false if receiving
     * @param frag XML fragment to print
     */
    virtual void printXml(const JBStream* stream, bool send, XmlFragment& frag) const;

protected:
    /**
     * Add a stream to one of the stream lists
     * @param stream The stream to add
     */
    virtual void addStream(JBStream* stream);

    /**
     * Remove a stream
     * @param stream The stream to remove
     * @param delObj True to release the stream, false to remove it from list
     *  without releasing it
     */
    virtual void removeStream(JBStream* stream, bool delObj = true);

    /**
     * Stop all stream sets
     * @param waitTerminate True to wait for all streams to terminate
     */
    virtual void stopStreamSets(bool waitTerminate = true)
	{}

    /**
     * Retrieve the list of streams of a given type.
     * Descendant must implement it
     * @param list The destination list to set
     * @param type Stream type
     */
    virtual void getStreamList(RefPointer<JBStreamSetList>& list, int type)
	{}

    /**
     * Retrieve all streams
     * @param list The destination list to set. The first index will be filled with the
     *  c2s streams list, the second index will be set to the s2s stream list
     * @param type Optional stream type
     */
    inline void getStreamLists(RefPointer<JBStreamSetList> list[JBStream::TypeCount],
	int type = JBStream::TypeCount) {
	    if (type == JBStream::c2s || type == JBStream::TypeCount)
		getStreamList(list[JBStream::c2s],JBStream::c2s);
	    if (type == JBStream::s2s || type == JBStream::TypeCount)
		getStreamList(list[JBStream::s2s],JBStream::s2s);
	}

    /**
     * Find a stream by its name in a given set list
     * @param id The name of the stream to find
     * @param list The list to search for a stream
     * @return Referenced JBStream pointer or 0
     */
    JBStream* findStream(const String& id, JBStreamSetList* list);

    bool m_exiting;                      // Engine exiting flag
    JBRemoteDomainDef m_remoteDomain;    // Default remote domain definition
    ObjList m_remoteDomains;             // Remote domain definitions
    unsigned char m_restartMax;          // Maximum value for stream restart counter
    unsigned int m_restartUpdInterval;   // Update interval for stream restart counter
    unsigned int m_setupTimeout;         // Overall stream setup timeout
    unsigned int m_startTimeout;         // Wait stream start period
    unsigned int m_connectTimeout;       // Outgoing: socket connect timeout
    unsigned int m_pingInterval;         // Stream idle interval (no data received)
    unsigned int m_pingTimeout;          // Sent ping timeout
    unsigned int m_idleTimeout;          // Stream idle timeout (nothing sent or received)
    unsigned int m_streamReadBuffer;     // Stream read buffer length
    unsigned int m_maxIncompleteXml;     // Maximum length of an incomplete xml
    int m_printXml;                      // Print XML data to output
    bool m_initialized;                  // True if already initialized

private:
    // Add/remove a connect stream thread when started/stopped
    void connectStatus(JBConnect* conn, bool started);

    ObjList m_connect;                   // Connecting streams
};

/**
 * This class implements a Jabber server engine
 * @short A Jabber server engine
 */
class YJABBER_API JBServerEngine : public JBEngine
{
    YCLASS(JBServerEngine,GenObject)
public:
    /**
     * Constructor
     * @param name Engine name
     */
    JBServerEngine(const char* name = "jbserverengine");

    /**
     * Destructor
     */
    ~JBServerEngine();

    /**
     * Terminate all streams. Stop all sets processors. Release memory if final
     * @param final True if called from destructor
     * @param waitTerminate True to wait for all streams to terminate
     */
    virtual void cleanup(bool final = false, bool waitTerminate = true);

    /**
     * Build an internal stream name
     * @param name Destination buffer
     */
    virtual void buildStreamName(String& name)
	{ name << "stream/" << getStreamIndex(); }

    /**
     * Find a server to server stream by local/remote domain.
     * Skip over outgoing dialback only streams
     * This method is thread safe
     * @param local Local domain
     * @param remote Remote domain
     * @param out True to find an outgoing stream, false to find an incoming one
     * @return Referenced JBServerStream pointer or 0
     */
    JBServerStream* findServerStream(const String& local, const String& remote, bool out);

    /**
     * Create an outgoing s2s stream.
     * @param local Local party domain
     * @param remote Remote party domain
     * @param dbId Optional dialback id (stream id)
     * @param dbKey Optional dialback key to verify
     * @param dbOnly True if this is a dialback only stream
     * @return Referenced JBServerStream pointer or 0 if a stream already exists
     */
    JBServerStream* createServerStream(const String& local, const String& remote,
	const char* dbId = 0, const char* dbKey = 0, bool dbOnly = false);

    /**
     * Terminate all incoming c2s streams matching a given JID
     * This method is thread safe
     * @param jid Client JID
     * @param error Optional error to be sent to the client
     * @param reason Optional error text to be sent to the client
     * @return The number of stream terminated
     */
    unsigned int terminateClientStreams(const JabberID& jid,
	XMPPError::Type error = XMPPError::NoError, const char* reason = 0);

protected:
    /**
     * Add a stream to one of the stream lists
     * @param stream The stream to add
     */
    virtual void addStream(JBStream* stream);

    /**
     * Remove a stream
     * @param stream The stream to remove
     * @param delObj True to release the stream, false to remove it from list
     *  without releasing it
     */
    virtual void removeStream(JBStream* stream, bool delObj = true);

    /**
     * Stop all stream sets
     * @param waitTerminate True to wait for all streams to terminate
     */
    virtual void stopStreamSets(bool waitTerminate = true);

    /**
     * Retrieve the list of streams of a given type
     * @param list The destination list to set
     * @param type Stream type
     */
    virtual void getStreamList(RefPointer<JBStreamSetList>& list, int type);

    /**
     * Increment and return the stream index counter
     * @return Current stream index
     */
    inline unsigned int getStreamIndex() {
	    Lock lock(this);
	    return ++m_streamIndex;
	}

    unsigned int m_streamIndex;          // Index used to build stream name
    JBStreamSetList* m_c2sReceive;       // c2s streams receive list
    JBStreamSetList* m_c2sProcess;       // c2s streams process list
    JBStreamSetList* m_s2sReceive;       // s2s streams receive list
    JBStreamSetList* m_s2sProcess;       // s2s streams process list
};

/**
 * This class implements a Jabber client engine
 * @short A Jabber client engine
 */
class YJABBER_API JBClientEngine : public JBEngine
{
    YCLASS(JBClientEngine,GenObject)
public:
    /**
     * Constructor
     * @param name Engine name
     */
    JBClientEngine(const char* name = "jbclientengine");

    /**
     * Destructor
     */
    ~JBClientEngine();

    /**
     * Terminate all streams. Stop all sets processors. Release memory if final
     * @param final True if called from destructor
     * @param waitTerminate True to wait for all streams to terminate
     */
    virtual void cleanup(bool final = false, bool waitTerminate = true);

    /**
     * Build an outgoing client stream
     * @param account Account (stream) name
     * @param params Stream parameters
     * @return Referenced JBClientStream pointer or 0 if a stream already exists
     */
    JBClientStream* create(const String& account, const NamedList& params);

protected:
    /**
     * Add a stream to one of the stream lists
     * @param stream The stream to add
     */
    virtual void addStream(JBStream* stream);

    /**
     * Remove a stream
     * @param stream The stream to remove
     * @param delObj True to release the stream, false to remove it from list
     *  without releasing it
     */
    virtual void removeStream(JBStream* stream, bool delObj = true);

    /**
     * Stop all stream sets
     * @param waitTerminate True to wait for all streams to terminate
     */
    virtual void stopStreamSets(bool waitTerminate = true);

    /**
     * Retrieve the list of streams of a given type
     * @param list The destination list to set
     * @param type Stream type
     */
    virtual void getStreamList(RefPointer<JBStreamSetList>& list, int type);

    JBStreamSetList* m_receive;          // Streams receive list
    JBStreamSetList* m_process;          // Streams process list
};

/**
 * This class holds a set of streams to be processed in an uniform way.
 * This is a base class for specialized stream list processors.
 * Its process() method should be called in its own thread
 * @short A set of streams to be processed in an uniform way
 */
class YJABBER_API JBStreamSet : public GenObject, public Mutex
{
    YCLASS(JBStreamSet,GenObject);
    friend class JBStreamSetList;
public:
    /**
     * Destructor. Delete the owned streams. Remove from owner
     */
    virtual ~JBStreamSet();

    /**
     * Retrieve the list of clients.
     * Make sure the set is locked before calling this method
     * @return The list of clients
     */
    inline ObjList& clients()
	{ return m_clients; }

    /**
     * Add a stream to the set. The stream's reference counter will be increased.
     * This method doesn't check if the stream is already added
     * @param client The stream to append
     * @return True on success, false if there is no more room in this set
     */
    virtual bool add(JBStream* client);

    /**
     * Remove a stream from set
     * @param client The stream to remove
     * @param delObj True to release the stream, false to remove it from list
     *  without releasing it
     * @return True on success, false if not found
     */
    virtual bool remove(JBStream* client, bool delObj = true);

    /**
     * Terminate all streams matching local/remote jid
     * @param local Optional local jid to match
     * @param remote Optional remote jid to match
     * @param error Optional error to be sent to the client
     * @param reason Optional error text to be sent to the client
     * @return The number of streams terminated
     */
    unsigned int dropAll(const JabberID& local = JabberID::empty(),
	const JabberID& remote = JabberID::empty(),
	XMPPError::Type error = XMPPError::NoError, const char* reason = 0);

    /**
     * Process the list.
     * Returns as soon as there are no more streams in the list
     */
    void run();

    /**
     * Start running
     * @return True on success
     */
    virtual bool start();

    /**
     * Stop running
     */
    virtual void stop();

protected:
    /**
     * Constructor
     * @param owner The list owning this set
     */
    JBStreamSet(JBStreamSetList* owner);

    /**
     * This method is called from run() with the list unlocked and stream's
     *  reference counter increased.
     * A specialized processor must implement this method
     * @param stream The stream to process
     * @return True if something was processed
     */
    virtual bool process(JBStream& stream) = 0;

    bool m_changed;                      // List changed flag
    bool m_exiting;                      // The thread is exiting (don't accept clients)
    JBStreamSetList* m_owner;            // The list owning this set
    ObjList m_clients;                   // The streams list

private:
    JBStreamSet() {}                     // Private default constructor (forbidden)
};


/**
 * This class holds a set specialized in stream processing
 * @short Specialized stream processor
 */
class YJABBER_API JBStreamSetProcessor : public JBStreamSet
{
    YCLASS(JBStreamSetProcessor,JBStreamSet);
protected:
    /**
     * Constructor
     * @param owner The list owning this set
     */
    inline JBStreamSetProcessor(JBStreamSetList* owner)
	: JBStreamSet(owner)
	{}

    /**
     * This method is called from run() with the list unlocked and stream's
     *  reference counter increased.
     * Calls stream's getEvent(). Pass a generated event to the engine
     * Remove the stream from its engine on destroy
     * @param stream The stream to process
     * @return True if an event was generated by the stream
     */
    virtual bool process(JBStream& stream);
};


/**
 * This class holds a set specialized in stream data receiver
 * @short Specialized stream data receiver
 */
class YJABBER_API JBStreamSetReceive : public JBStreamSet
{
    YCLASS(JBStreamSetReceive,JBStreamSet);
protected:
    /**
     * Constructor. Build the read buffer
     * @param owner The list owning this set
     */
    JBStreamSetReceive(JBStreamSetList* owner);

    /**
     * This method is called from run() with the list unlocked and stream's
     *  reference counter increased.
     * Calls stream's readSocket()
     * @param stream The stream to process
     * @return True if the stream received any data
     */
    virtual bool process(JBStream& stream);

protected:
    DataBlock m_buffer;                  // Read buffer
};


/**
 * This class holds a list of stream sets.
 * The purpose is to create a list of threads 
 * @short A list of stream sets
 */
class YJABBER_API JBStreamSetList : public RefObject, public Mutex
{
    YCLASS(JBStreamSetList,RefObject);
    friend class JBStreamSet;
public:
    /**
     * Constructor
     * @param engine Engine owning this list
     * @param max Maximum streams per set (0 for maximum possible)
     * @param sleepMs Time to sleep when idle
     * @param name List name (for debugging purposes)
     */
    JBStreamSetList(JBEngine* engine, unsigned int max, unsigned int sleepMs,
	const char* name);

    /**
     * Retrieve the stream set list.
     * Make sure the list is locked before calling this method
     * @return The stream set list
     */
    inline ObjList& sets()
	{ return m_sets; }

    /**
     * Destructor
     */
    virtual ~JBStreamSetList();

    /**
     * Retrieve the maximum number of streams per set
     * @return The maximum number of streams per set
     */
    inline unsigned int max() const
	{ return m_max; }

    /**
     * Retrieve the number of streams in all sets
     * @return The number of streams in all sets
     */
    inline unsigned int streamCount() const
	{ return m_streamCount; }

    /**
     * Retrieve the engine owning this list
     * @return The engine owning this list
     */
    inline JBEngine* engine() const
	{ return m_engine; }

    /**
     * Add a stream to the list. Build a new set if there is no room in existing sets
     * @param client The stream to add
     * @return True on success
     */
    bool add(JBStream* client);

    /**
     * Remove a stream from list
     * @param client The stream to remove
     * @param delObj True to release the stream, false to remove it from list
     *  without releasing it
     */
    void remove(JBStream* client, bool delObj = true);

    /**
     * Stop one set or all sets
     * @param set The set to stop, 0 to stop all
     * @param waitTerminate True to wait for all streams to terminate
     */
    void stop(JBStreamSet* set = 0, bool waitTerminate = true);

    /**
     * Get the string representation of this list
     * @return The list name
     */
    virtual const String& toString() const;

protected:
    /**
     * Stop all sets. Release memory
     */
    virtual void destroyed();

    /**
     * Remove a set from list without deleting it
     * @param set The set to remove
     */
    void remove(JBStreamSet* set);

    /**
     * Build a specialized stream set. Descendants must override this method
     * @return JBStreamSet pointer or 0
     */
    virtual JBStreamSet* build();

    JBEngine* m_engine;                  // The engine owning this list
    String m_name;                       // List name
    unsigned int m_max;                  // The maximum number of streams per set
    unsigned int m_sleepMs;              // Time to sleep if nothig processed
    ObjList m_sets;                      // The sets list

private:
    JBStreamSetList() {}                 // Private default constructor (forbidden)

    unsigned int m_streamCount;          // Current number of streams in this list
};


/**
 * This class holds entity capability data
 * Implements XEP 0115 support
 * @short Entity capability
 */
class YJABBER_API JBEntityCaps : public String
{
    YCLASS(JBEntityCaps,String);
public:
    /**
     * Supported XEP 0115 versions
     */
    enum {
	Ver1_3 = 1,  // Version lower then 1.4 (m_data is the node version + advertised extensions)
	Ver1_4 = 2,  // Version 1.4 or greater (m_data is the SHA-1 hash of features and identities)
    };

    /**
     * Constructor
     * @param id Object id
     * @param version Entity caps version
     * @param node Entity node
     * @param data Entity data
     */
    inline JBEntityCaps(const char* id, char version, const char* node, const char* data)
	: String(id),
	m_version(version), m_node(node), m_data(data)
	{}

    /**
     * Build an entity caps id
     * @param buf Destination buffer
     * @param version Entity caps version
     * @param node Entity node
     * @param data Entity data
     * @param ext Optional entity extensions
     */
    static inline void buildId(String& buf, char version, const char* node,
	const char* data, String* ext = 0)
	{ buf << (int)version << node << data << (ext ? ext->c_str() : ""); }

    char m_version;
    String m_node;
    String m_data;
    XMPPFeatureList m_features;

private:
    JBEntityCaps() {}
};


/**
 * This class holds data and offer entity capability services.
 * Implements XEP 0115 support
 * @short Entity capability list manager
 */
class YJABBER_API JBEntityCapsList : public ObjList, public Mutex
{
    YCLASS(JBEntityCapsList,ObjList);
public:
    /**
     * Constructor
     */
    inline JBEntityCapsList()
	: Mutex(true,"JBEntityCapsList"), m_enable(true), m_reqIndex(0)
	{ m_reqPrefix << "xep0115" << (unsigned int)Time::msecNow() << "_"; }

    /**
     * Retrieve an entity caps object. This method is not thread safe
     * @param id The id to find
     * @return JBEntityCaps pointer or 0
     */
    inline JBEntityCaps* findCaps(const String& id) {
	    for (ObjList* o = skipNull(); o; o = o->skipNext())
		if (o->get()->toString() == id)
		    return static_cast<JBEntityCaps*>(o->get());
	    return 0;
	}

    /**
     * Expire pending requests.
     * This method is thread safe
     * @param msecNow Current time
     */
    void expire(u_int64_t msecNow = Time::msecNow());

    /**
     * Process a response.
     * This method is thread safe
     * @param rsp The element to process
     * @param id The element's id
     * @param ok True if the response is a result one, false if it's an error
     * @return True if the element was processed (handled)
     */
    bool processRsp(XmlElement* rsp, const String& id, bool ok);

    /**
     * Request entity capabilities.
     * This method is thread safe
     * @param stream The stream to send the request
     * @param from The 'from' attribute
     * @param to The 'to' attribute
     * @param id Entity caps id
     * @param version Entity caps version
     * @param node Entity node
     * @param data Entity caps data
     */
    void requestCaps(JBStream* stream, const char* from, const char* to, const String& id,
	char version, const char* node, const char* data);

    /**
     * Build an XML document from this list.
     * This method is thread safe
     * @param rootName Document root element name
     * @return XmlDocument pointer
     */
    XmlDocument* toDocument(const char* rootName = "entitycaps");

    /**
     * Build this list from an XML document.
     * This method is thread safe
     * @param doc Document to build from
     * @param rootName Document root element name (it will be checked if set)
     * @return XmlDocument pointer
     */
    void fromDocument(XmlDocument& doc, const char* rootName = "entitycaps");

    /**
     * Process an element containing an entity capabily child.
     * Request capabilities if not found in the list.
     * This method is thread safe
     * @param capsId String to be filled with entity caps object id
     *  (empty if an entity caps child is not found in element )
     * @param xml XML element to process
     * @param stream The stream used to request capabilities
     * @param from The 'from' attribute of the request stanza
     * @param to The 'to' attribute of the request stanza
     * @return XmlDocument pointer
     */
    virtual void processCaps(String& capsId, XmlElement* xml, JBStream* stream,
	const char* from, const char* to);

    /**
     * Add capabilities to a list.
     * This method is thread safe
     * @param list Destination list
     * @param id Entity caps id
     */
    inline void addCaps(NamedList& list, const String& id) {
	    Lock lock(this);
	    JBEntityCaps* caps = findCaps(id);
	    if (caps)
		addCaps(list,*caps);
	}

    /**
     * Add capabilities to a list.
     * This method is not thread safe
     * @param list Destination list
     * @param caps Entity caps to add
     */
    virtual void addCaps(NamedList& list, JBEntityCaps& caps);

    /**
     * Load (reset) this list from an XML document file.
     * This method is thread safe
     * @param file The file to load
     * @param enabler The debug enabler used to output messages
     * @return True on success
     */
    bool loadXmlDoc(const char* file, DebugEnabler* enabler = 0);

    /**
     * Save this list to an XML document file.
     * This method is thread safe
     * @param file The file to save
     * @param enabler The debug enabler used to output messages
     * @return True on success
     */
    bool saveXmlDoc(const char* file, DebugEnabler* enabler = 0);

    /**
     * Check if an XML element has a 'c' entity capability child and decode it
     * @param xml The element to process
     * @param version Entity caps version
     * @param node Entity node attribute
     * @param ver Entity ver attribute
     * @param ext Entity ext attribute if version is less the 1.4
     * @return True if a child was succesfully decoded
     */
    static bool decodeCaps(const XmlElement& xml, char& version, String*& node,
	String*& ver, String*& ext);

    /**
     * Enabled flag
     */
    bool m_enable;

protected:
    /**
     * Caps list item add notification for descendants.
     * This method is called when processing responses with the list locked
     * @param caps Changed caps object. 0 if none specified
     */
    virtual void capsAdded(JBEntityCaps* caps)
	{}

    unsigned int m_reqIndex;             // Disco info request index
    String m_reqPrefix;                  // Prefix for disco info stanza id
    ObjList m_requests;                  // List of sent disco info requests
};

}; // namespace TelEngine

#endif /* __YATEJABBER_H */

/* vi: set ts=8 sw=4 sts=4 noet: */