// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "base/smart_ptr.h"
#include "gameswf_string.h"

#ifdef HAVE_LIBXML

#include "gameswf_xml.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

namespace gameswf
{

array<as_object *> _xmlobjs;    // FIXME: hack alert

XMLAttr::XMLAttr()
{
  //log_msg("%s: %p \n", __FUNCTION__, this);
}

XMLAttr::~XMLAttr()
{
  //log_msg("%s: %p \n", __FUNCTION__, this);
}
  
XMLNode::XMLNode()
{
  //log_msg("%s: %p \n", __FUNCTION__, this);
}

XMLNode::~XMLNode()
{
  //log_msg("%s: %p \n", __FUNCTION__, this);

#if 0
  for (i=0; i<_children.size(); i++) {
    delete _children[i];
  }
  _children.clear();
    
  for (i=0; i<_attributes.size(); i++) {
    delete _attributes[i];
  }
  _attributes.clear();
#endif
}

XML::XML()
{
  //log_msg("%s: %p \n", __FUNCTION__, this);
  _loaded = false;
  _nodename = 0;
}


// Parse the ASCII XML string into memory
XML::XML(tu_string xml_in)
{
  //log_msg("%s: %p \n", __FUNCTION__, this);
  memset(&_nodes, 0, sizeof(XMLNode));
  parseXML(xml_in);
}

XML::XML(struct node *childNode)
{
  //log_msg("%s: %p \n", __FUNCTION__, this);
}


XML::~XML()
{
  //log_msg("%s: %p \n", __FUNCTION__, this);
  //delete _nodes;
}

// Dispatch event handler(s), if any.
bool
XML::on_event(event_id id)
{
  // Keep m_as_environment alive during any method calls!
  //  smart_ptr<as_object_interface>	this_ptr(this);
  
#if 0
  // First, check for built-in event handler.
  as_value	method;
  if (get_event_handler(event_id(id), &method))
    {/
      // Dispatch.
      call_method0(method, &m_as_environment, this);
          return true;
    }
  
  // Check for member function.
  // In ActionScript 2.0, event method names are CASE SENSITIVE.
  // In ActionScript 1.0, event method names are CASE INSENSITIVE.
  const tu_string&	method_name = id.get_function_name();
  if (method_name.length() > 0)hostByNameGet
    {
      as_value	method;
      if (get_member(method_name, &method))
        {
          call_method0(method, &m_as_environment, this);
          return true;
        }
    }
#endif
  return false;
}

void
XML::on_event_load()
{
  // Do the events that (appear to) happen as the movie
  // loads.  frame1 tags and actions are executed (even
  // before advance() is called).  Then the onLoad event
  // is triggered.
  {
    on_event(event_id::LOAD);
  }
}

XMLNode*
XML::extractNode(xmlNodePtr node, bool mem)
{
  xmlAttrPtr attr;
  xmlNodePtr childnode;
  xmlChar *ptr = NULL;
  XMLNode *element, *child;

  element = new XMLNode;
  //log_msg("Created new element for %s at %p\n", node->name, element);
  memset(element, 0, sizeof (XMLNode));

  //log_msg("%s: extracting node %s\n", __FUNCTION__, node->name);

  // See if we have any Attributes (properties)
  attr = node->properties;
  while (attr != NULL) {
    //log_msg("extractNode %s has property %s, value is %s\n",
    //          node->name, attr->name, attr->children->content);
    XMLAttr *attrib = new XMLAttr;
    attrib->_name = reinterpret_cast<const char *>(attr->name);
    attrib->_value = reinterpret_cast<const char *>(attr->children->content);
    //log_msg("\tPushing attribute %s for element %s has value %s\n",
    //        attr->name, node->name, attr->children->content);
    element->_attributes.push_back(attrib);
    attr = attr->next;
  }

  element->_name = reinterpret_cast<const char *>(node->name);
  if (node->children) {
    //ptr = node->children->content;
    ptr = xmlNodeGetContent(node->children);
    if (ptr != NULL) {
      if ((strchr((const char *)ptr, '\n') == 0) && (ptr[0] != 0))
      {
        if (node->children->content == NULL) {
          //log_msg("Node %s has no contents\n", node->name);
        } else {
          //log_msg("extractChildNode from text for %s has contents %s\n", node->name, ptr);
          element->_value = reinterpret_cast<const char *>(ptr);
        }
      }
      xmlFree(ptr);
    }
  }

#if 0
  if (node->parent->type == XML_DOCUMENT_NODE) {
    if (mem) {
      //child = new XMLNode;
      //memset(child, 0, sizeof (XMLNode));
      if (node->type  == XML_ELEMENT_NODE) {
        child = new XMLNode;
        child->_name  = reinterpret_cast<const char *>(node->name);
        child->_attributes = element->_attributes;
        //log_msg("extractChildNode for %s has contents %s\n", node->name, ptr);
        //log_msg("Pushing Top Node %s for element %p\n", child->_name.c_str(), element);
        //child->_children.push_back();
        element->_children.push_back(child);
      }
    }
  }
#endif
  
  // See if we have any data (content)
  childnode = node->children;

  while (childnode != NULL) {
    if (childnode->type == XML_ELEMENT_NODE) {
      //log_msg("\t\t extracting node %s\n", childnode->name);
      child = extractNode(childnode, mem);
      if (child->_value.get_type() != as_value::UNDEFINED) {
        //log_msg("\tPushing childNode %s, value %s on element %p\n", child->_name.c_str(), child->_value.to_string(), element);
      } else {
        //log_msg("\tPushing childNode %s on element %p\n", child->_name.c_str(), element);
      }
      element->_children.push_back(child);
    }
    childnode = childnode->next;
  }

  return element;
}

// Read in an XML document from the specified source
bool
XML::parseDoc(xmlDocPtr document, bool mem)
{
  XMLNode *top;
  xmlNodePtr cur;

  //log_msg("%s: mem is %d\n", __FUNCTION__, mem);
  
  if (document == 0) {
    log_error("Can't load XML file!\n");
    return false;
  }

  cur = xmlDocGetRootElement(document);

  if (cur != NULL) {
    top = extractNode(cur, mem);
    //_nodes->_name = reinterpret_cast<const char *>(cur->name);
    _nodes = top;
    //_node_data.push_back(top);
    //cur = cur->next;
  }
  
  _loaded = true;
  return true;
}

// This reads in an XML file from disk and parses into into a memory resident
// tree which can be walked through later.
bool
XML::parseXML(tu_string xml_in)
{
  //log_msg("Parse XML from memory: %s\n", xml_in.c_str());

  if (xml_in.size() == 0) {
    log_error("XML data is empty!\n");
    return false;
  }
  
  _doc = xmlParseMemory(xml_in.c_str(), xml_in.size());
  if (_doc == 0) {
    log_error("Can't parse XML data!\n");
    return false;
  }
  
  bool ret = parseDoc(_doc, true);
  xmlFreeDoc(_doc);
  return ret;
}

// This reads in an XML file from disk and parses into into a memory resident
// tree which can be walked through later.
bool
XML::load(const char *filespec)
{
  
  log_msg("Load XML file: %s\n", filespec);
  _doc = xmlParseFile(filespec);

  if (_doc == 0) {
    log_error("Can't load XML file: %s!\n", filespec);
    return false;
  }

  bool ret = parseDoc(_doc, false);
  xmlFreeDoc(_doc);
  return ret;
}

bool
XML::onLoad()
{
  log_msg("%s: FIXME: onLoad Default event handler\n", __FUNCTION__);

  return(_loaded);
}

XMLNode *
XML::operator [] (int x) {
  log_msg("%s:\n", __FUNCTION__);

  return _nodes->_children[x];
}

void
XML::cleanupStackFrames(as_object *xml, as_environment *env)
{
}

as_object *
XML::setupFrame(as_object *obj, XMLNode *xml, bool mem)
{
  int           child, i;
  const char    *nodename;
  as_value      nodevalue;
  int           length;
  as_value      inum;
  XMLNode       *childnode;
  xmlnode_as_object *xmlchildnode_obj;
  xmlnode_as_object *xmlnode_obj;
  xmlattr_as_object* attr_obj;

#if 0
  log_msg("%s: processing node %s for object %p, mem is %d\n", __FUNCTION__, xml->_name.c_str(), obj, mem);
#endif
  
  xmlnode_as_object *xobj = (xmlnode_as_object *)obj;
  
  // Get the data for this node
  nodename   = xml->_name;
  nodevalue  = xml->_value;
  length     = xml->length();

  // Set these members in the top level object passed in. This are used
  // primarily by the disk based XML parser, where at least in all my current
  // test cases this is referenced with firstChild first, then nodeName and
  // childNodes.
  //obj->set_member("firstChild",         xobj);
  //obj->set_member("childNodes",         xobj);
  obj->set_member("nodeName",           nodename);
  obj->set_member("length",             length);
  if (nodevalue.get_type() != as_value::UNDEFINED) {
    tu_string_as_object *val_obj = new tu_string_as_object;
    val_obj->str = nodevalue.to_string();
    //obj->set_member("nodeValue",        val_obj);
    obj->set_member("nodeValue",        nodevalue.to_string());
    //obj->set_member("nodeValue",        nodevalue);
    //log_msg("\tnodevalue for %s is: %s\n", nodename, nodevalue.to_string());
  } else {
    // If there is no value, we want to define an empty
    // object. otherwise we wind up with an "undefined" object anyway,
    // which gets displayed.
    tu_string_as_object *val_obj = new tu_string_as_object;
    val_obj->str = nodevalue.to_string();
    obj->set_member("nodeValue", "");
  }
  
  
  // Process the attributes, if any
  attr_obj = new xmlattr_as_object;
  for (i=0; i<xml->_attributes.size(); i++) {
    attr_obj->set_member(xml->_attributes[i]->_name, xml->_attributes[i]->_value);
//     log_msg("\tCreated attribute %s, value is %s\n",
//             xml->_attributes[i]->_name.c_str(),
//             xml->_attributes[i]->_value.to_string());
  }
  obj->set_member("attributes", attr_obj);

  //
  // Process the children, if there are any
  if (length) {
    //log_msg("\tProcessing %d children nodes for %s\n", length, nodename);
    for (child=0; child<length; child++) {
      // Create a new AS object for this node's children
      xmlchildnode_obj = new xmlnode_as_object;
      // When parsing XML from memory, the test movies I have expect the firstChild
      // to be the first element of the array instead.
      if (mem) {
        childnode = xml;
      } else {
        childnode = xml->_children[child];
      }
      xmlnode_obj = (xmlnode_as_object *)setupFrame(xmlchildnode_obj, childnode, false); // call ourself
      inum = child;
      obj->set_member(inum.to_string(), xmlnode_obj);
    }
  
  } else {
    //log_msg("\tNode %s has no children\n", nodename);
  }
  
  return obj;
}
  
//
// Callbacks. These are the wrappers for the C++ functions so they'll work as
// callbacks from within gameswf.
//
void
xml_load(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
{
  as_value	method;
  as_value	val;
  bool          ret;
  struct stat   stats;


  //log_msg("%s:\n", __FUNCTION__);
  
  xml_as_object *xml_obj = (xml_as_object*) (as_object*) this_ptr;
  
  assert(ptr);
  const tu_string filespec = env->bottom(first_arg).to_string();

  // If the file doesn't exist, don't try to do anything.
  if (stat(filespec.c_str(), &stats) < 0) {
    fprintf(stderr, "ERROR: doesn't exist.%s\n", filespec.c_str());
    result->set(false);
    return;
  }
  
  // Set the argument to the function event handler based on whether the load
  // was successful or failed.
  ret = xml_obj->obj.load(filespec);
  result->set(ret);

  if (ret == false) {
    return;
  }
    
  //env->bottom(first_arg) = ret;
  array<with_stack_entry> with_stack;
  array<with_stack_entry> dummy_stack;
  //  struct node *first_node = ptr->obj.firstChildGet();
  
  //const char *name = ptr->obj.nodeNameGet();

  if (xml_obj->obj.hasChildNodes() == false) {
    log_error("%s: No child nodes!\n", __FUNCTION__);
  }  
  xml_obj->obj.setupFrame(xml_obj, xml_obj->obj.firstChild(), false);
  
#if 1
  if (this_ptr->get_member("onLoad", &method)) {
    //    log_msg("FIXME: Found onLoad!\n");
    env->set_variable("success", true, 0);
    env->bottom(first_arg) = true;
    as_c_function_ptr	func = method.to_c_function();
    if (func)
      {
        // It's a C function.  Call it.
        log_msg("Calling C function for onLoad\n");
        (*func)(&val, xml_obj, env, nargs, first_arg); // was this_ptr instead of node
      }
    else if (as_as_function* as_func = method.to_as_function())
      {
        // It's an ActionScript function.  Call it.
        log_msg("Calling ActionScript function for onLoad\n");
        (*as_func)(&val, xml_obj, env, nargs, first_arg); // was this_ptr instead of node
      } else {
        log_error("error in call_method(): method is not a function\n");
      }
  } else {
    log_msg("Couldn't find onLoad event handler, setting up callback\n");
    // ptr->set_event_handler(event_id::XML_LOAD, (as_c_function_ptr)&xml_onload);
  }
#else
  xml_obj->set_event_handler(event_id::XML_LOAD,
                               (as_c_function_ptr)&xml_onload);

#endif

  result->set(true);
}

// This executes the event handler for XML::XML_LOAD if it's been defined,
// and the XML file has loaded sucessfully.
void
xml_onload(as_value* result, as_object_interface* this_ptr, as_environment* env)
{
  log_msg("%s:\n", __FUNCTION__);
    
  as_value	method;
  as_value      val;
  static bool first = true;     // This event handler should only be executed once.
  array<with_stack_entry>	empty_with_stack;
  xml_as_object*	ptr = (xml_as_object*) (as_object*) this_ptr;
  assert(ptr);
  
  if ((ptr->obj.loaded()) && (first)) {
    // env->set_variable("success", true, 0);
    //as_value bo(true);
    //env->push_val(bo);

    first = false;
    log_msg("The XML file has been loaded successfully!\n");
    // ptr->on_event(event_id::XML_LOAD);
    //env->set_variable("success", true, 0);
    //env->bottom(0) = true;
    
    if (this_ptr->get_member("onLoad", &method)) {
      // log_msg("FIXME: Found onLoad!\n");
      as_c_function_ptr	func = method.to_c_function();
      if (func)
        {
          // It's a C function.  Call it.
          log_msg("Calling C function for onLoad\n");
          (*func)(&val, this_ptr, env, 0, 0);
        }
      else if (as_as_function* as_func = method.to_as_function())
        {
          // It's an ActionScript function.  Call it.
          log_msg("Calling ActionScript function for onLoad\n");
        (*as_func)(&val, this_ptr, env, 0, 0);
        }
      else
        {
          log_error("error in call_method(): method is not a function\n");
        }    
    } else {
      log_msg("FIXME: Couldn't find onLoad!\n");
    }
  }
      
  result->set(&val);
}

// This is the default event handler, and is usually redefined in the SWF script
void
xml_ondata(as_value* result, as_object_interface* this_ptr, as_environment* env)
{
   log_msg("%s:\n", __FUNCTION__);
    
  as_value	method;
  as_value	val;
  static bool first = true;     // FIXME: ugly hack!
  
  xml_as_object*	ptr = (xml_as_object*) (as_object*) this_ptr;
  assert(ptr);
  
  if ((ptr->obj.loaded()) && (first)) {
    if (this_ptr->get_member("onData", &method)) {
      log_msg("FIXME: Found onData!\n");
      as_c_function_ptr	func = method.to_c_function();
      env->set_variable("success", true, 0);
      if (func)
        {
          // It's a C function.  Call it.
          log_msg("Calling C function for onData\n");
          (*func)(&val, this_ptr, env, 0, 0);
      }
      else if (as_as_function* as_func = method.to_as_function())
        {
          // It's an ActionScript function.  Call it.
          log_msg("Calling ActionScript function for onData\n");
          (*as_func)(&val, this_ptr, env, 0, 0);
        }
      else
        {
          log_error("error in call_method(): method is not a function\n");
        }    
    } else {
      log_msg("FIXME: Couldn't find onData!\n");
    }
  }

  result->set(&val);
}

void
xml_new(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
{
  as_value      inum;
  xml_as_object *xml_obj;
  
  //log_msg("%s: nargs=%d\n", __FUNCTION__, nargs);
  
  if (nargs > 0) {
    if (env->top(0).get_type() == as_value::STRING) {
      xml_obj = new xml_as_object;
      //log_msg("\tCreated New XML object at %p\n", xml_obj);
      tu_string datain = env->top(0).to_tu_string();
      xml_obj->obj.parseXML(datain);
      xml_obj->obj.setupFrame(xml_obj, xml_obj->obj.firstChild(), true);
    } else {
      xml_as_object*	xml_obj = (xml_as_object*)env->top(0).to_object();      
      //log_msg("\tCloned the XML object at %p\n", xml_obj);
      //result->set(xml_obj);
      result->set_as_object_interface(xml_obj);
      return;
    }
  } else {
    xml_obj = new xml_as_object;
    //log_msg("\tCreated New XML object at %p\n", xml_obj);
    xml_obj->set_member("load", &xml_load);
    xml_obj->set_member("loaded", &xml_loaded);
  }

  _xmlobjs.push_back((as_object *)xml_obj);

  //result->set(xml_obj);
  result->set_as_object_interface(xml_obj);
}

//
// SWF Property of this class. These are "accessors" into the private data
// of the class.
//

// determines whether the document-loading process initiated by the XML.load()
// call has completed. If the process completes successfully, the method
// returns true; otherwise, it returns false.
void
xml_loaded(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
{
  as_value	method;
  as_value	val;

  log_msg("%s:\n", __FUNCTION__);
    
  xml_as_object*	ptr = (xml_as_object*) (as_object*) this_ptr;
  assert(ptr);
  tu_string filespec = env->bottom(first_arg).to_string();
  result->set(ptr->obj.loaded());
}

// References the first child in the parent node's children list. This
// property is null if the node does not have children. This property
// is undefined if the node is a text node.
void
xml_firstchild(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
{
  as_value	method;
  as_value	val;

  log_msg("%s:\n", __FUNCTION__);
    
  xml_as_object*	ptr = (xml_as_object*) (as_object*) this_ptr;
  assert(ptr);
  tu_string filespec = env->bottom(first_arg).to_string();
  result->set(ptr->obj.firstChild());
}

// an array of the specified XML object's children. Each element in the
// array is a reference to an XML object that represents a child node.
void
xml_childnodes(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
{
  as_value	method;
  as_value	val;

  log_msg("%s:\n", __FUNCTION__);
    
  // xml_as_object*	ptr = (xml_as_object*) (as_object*) this_ptr;
  // assert(ptr);
  tu_string filespec = env->bottom(first_arg).to_string();
  //  result->set(ptr->obj.childNodesGet());
}

void
xml_nodename(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg)
{
  as_value	method;

  log_msg("%s:\n", __FUNCTION__);
    
  // xml_as_object*	ptr = (xml_as_object*) (as_object*) this_ptr;
  // assert(ptr);
  tu_string filespec = env->bottom(first_arg).to_string();
  //  result->set(ptr->obj.nodeNameGet().c_str());
}

void xml_next_stack_depth(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg)
{
  as_value	method;
  as_value	val;

  log_msg("%s:\n", __FUNCTION__);
  array<with_stack_entry> with_stack;
    
  // xml_as_object*	ptr = (xml_as_object*) (as_object*) this_ptr;
  // assert(ptr);
  // double index = env->bottom(first_arg).to_number();
  //  ptr->obj.currentSet(index);
  //  env->set_variable("nodeName", ptr->obj.nodeNameGet(), with_stack);
}

} // end of gameswf namespace

// HAVE_LIBXML
#endif
