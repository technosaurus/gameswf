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

XMLNode::XMLNode()
{
}

XMLNode::~XMLNode()
{
  log_msg("%s: \n", __FUNCTION__);
}


XML::XML()
{
  _loaded = false;
  _nodename = 0;
}


// Parse the ASCII XML string into memory
XML::XML(tu_string xml_in)
{
  memset(&_nodes, 0, sizeof(XMLNode));
  parseXML(xml_in);
}

XML::XML(struct node *childNode)
{
  log_msg("FIXME: %s\n", __FUNCTION__);  
}


XML::~XML()
{
  log_msg("%s: \n", __FUNCTION__);
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
XML::extractNode(xmlNodePtr node)  
{
  xmlAttrPtr attr;
  xmlNodePtr childnode, next;
  xmlChar *ptr;
  XMLNode *element, *child;

  element = new XMLNode;
  memset(element, 0, sizeof (XMLNode));

  //log_msg("%s: extracting node %s\n", __FUNCTION__, node->name);
  if (xmlStrcmp(node->name, (const xmlChar *)"text") == 0) {
    //  node = node->next;
  }
  
  // See if we have any data (content)
  childnode = node->children;
  while (childnode != NULL) {
    // This block of code is only used with text based XML files.
    if (xmlStrcmp(childnode->name, (const xmlChar *)"text") == 0) {
      //log_msg("Getting the next childnode!\n");
      next = childnode->next;
      if (next != 0) {
        child = extractNode(next);
        //log_msg("Pushing childNode1 %s after extractNode()\n", child->_name.c_str());
        element->_children.push_back(child);
      } else {
        ptr = childnode->content;
        if (ptr != NULL) {
          child = new XMLNode;
          memset(child, 0, sizeof (XMLNode));
          // log_msg("extractChildNode from text for %s has contents %s\n", node->name, ptr);
          child->_name  = reinterpret_cast<const char *>(node->name);
          child->_value = reinterpret_cast<const char *>(ptr);
          //log_msg("Pushing childNode %s\n", child->_name.c_str());
          element->_children.push_back(child);
        }
      }
    }
    // This block of code is used by the network messages.
    else {
      if (xmlStrcmp(node->name, (const xmlChar *)"text") == 0) {
        next = node->next;
      } else {
        next = node->children;
      }
      if (next != 0) {
        child = extractNode(next);
        if (strcmp(child->_name.c_str(), "text") != 0) {
          //log_msg("Pushing childNode2 %s after extractNode()\n", child->_name.c_str());
          element->_children.push_back(child);
        }
      } else {
        next = childnode->children;
        if (next != 0) {
          if (xmlStrcmp(next->name, (const xmlChar *)"text") != 0) {
            child = extractNode(next);
            //log_msg("Pushing childNode3 %s\n", childnode->name);
            element->_children.push_back(child);
          } else {
            ptr = next->content;
            if (ptr != NULL) {
              child = new XMLNode;
              memset(child, 0, sizeof (XMLNode));
              //log_msg("extractChildNode2 from text for %s has contents %s\n", childnode->name, ptr);
              child->_name  = reinterpret_cast<const char *>(childnode->name);
              child->_value = reinterpret_cast<const char *>(ptr);
              //log_msg("Pushing childNode4 %s\n", childnode->name);
              element->_children.push_back(child);
            }
          }
        } else {
          attr = node->properties;
          while (attr != NULL) {
            //log_msg("extractNode %s has property %s, value is %s\n", node->name, attr->name, attr->children->content);
            XMLAttr *attrib = new XMLAttr;
            attrib->_name = reinterpret_cast<const char *>(attr->name);
            attrib->_value = reinterpret_cast<const char *>(attr->children->content);
            element->_attributes.push_back(attrib);
            attr = attr->next;
          }
        }
      }
    }
    childnode = childnode->next;
  }
  
  // See if we have any Attributes (properties)
  attr = node->properties;
  while (attr != NULL) {
    //log_msg("extractNode %s has property %s, value is %s\n", node->name, attr->name, attr->children->content);
    XMLAttr *attrib = new XMLAttr;
    attrib->_name = reinterpret_cast<const char *>(attr->name);
    attrib->_value = reinterpret_cast<const char *>(attr->children->content);
    element->_attributes.push_back(attrib);
    attr = attr->next;
  }

  //
  element->_name = reinterpret_cast<const char *>(node->name);
  if (node->children) {
    ptr = node->children->content;
    if (ptr != NULL) {
      element->_value = reinterpret_cast<const char *>(ptr);
    }
  } else {
    ptr = node->content;
    if (ptr != NULL) {
      element->_value = reinterpret_cast<const char *>(ptr);
    }
  }
  
  return element;
}

// Read in an XML document from the specified source
bool
XML::parseDoc(xmlDocPtr document)
{
#if 0
  // struct node *element, *child, *first;
  XMLNode *element, *child, *grandchild;
  xmlNodePtr children;
  xmlNodePtr lastchild;
  const xmlChar *tmpstr;
  xmlAttrPtr attr;
#endif
  XMLNode *top;
  xmlNodePtr cur;

  if (document == 0) {
    log_error("Can't load XML file!\n");
    return false;
  }

  cur = xmlDocGetRootElement(document);

#if 1
  if (cur != NULL) {
    top = extractNode(cur);
    //_nodes->_name = reinterpret_cast<const char *>(cur->name);
    _nodes = top;
    //cur = cur->next;
  }
#else
      // nuke everything

  _firstChild = cur;
  log_msg("Adding First element %s\n", cur->name);
  XMLNode *xmlnode = new XMLNode;
  xmlnode->_name = reinterpret_cast<const char *>(cur->name);
  _nodename = xmlnode->_name;   // FIXME:

  _nodes._name = xmlnode->_name;
  attr = cur->properties;
  while (attr != NULL) {
    log_msg("Node %s has property %s, value is %s\n",
            _nodes._name.c_str(), attr->name, attr->children->content);
    XMLAttr *attrib = new XMLAttr;
    attrib->_name = reinterpret_cast<const char *>(attr->name);
    attrib->_value = reinterpret_cast<const char *>(attr->children->content);
    _nodes._attributes.push_back(attrib);
    attr = attr->next;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {

    if ((xmlStrcmp(cur->name, (const xmlChar *)"text"))) {
#if 0
      test = extractNode(cur); // FIXME: testing this only
#endif
    // element = new struct node; // FIXME:
      element = new XMLNode;
      element->_name = reinterpret_cast<const char *>(cur->name);
      tmpstr = xmlNodeListGetString(_doc, cur->xmlChildrenNode, 1);
      if (tmpstr != 0) {
        element->_value = reinterpret_cast<const char *>(tmpstr);
        
      }
      attr = cur->properties;
      while (attr != NULL) {
        log_msg("Childnode %s has property %s, value is %s\n",
                element->_name.c_str(), attr->name, attr->children->content);
        XMLAttr *attrib = new XMLAttr;
        attrib->_name = reinterpret_cast<const char *>(attr->name);
        attrib->_value = reinterpret_cast<const char *>(attr->children->content);
        element->_attributes.push_back(attrib);
        attr = attr->next;
      }
      
      log_msg("Adding element %s\n", element->_name.c_str());
      //  _firstNode.children.push_back(element);

      children = cur->xmlChildrenNode;

      while (children != NULL) {
        if ((xmlStrcmp(children->name, (const xmlChar *)"text"))) {
#if 0
          child = extractNode(children); // FIXME: testing this only
#else   
          // child = new struct node;
          child = new XMLNode;
          child->_name = reinterpret_cast<const char *>(children->name);
          
          //tmpstr = xmlNodeListGetString(_doc, children->xmlChildrenNode, 1);
          
          //if (tmpstr != 0)          
          if (children->children->children)
            {
              //tmpstr = xmlNodeListGetString(_doc, children->xmlChildrenNode, 1);
              //child->_value = reinterpret_cast<const char *>(tmpstr);
            log_msg("child %s has data %s\n", child->_name.c_str(), child->_value.to_string());

            lastchild = children->xmlChildrenNode;
            while (lastchild != NULL) {
              if ((xmlStrcmp(lastchild->name, (const xmlChar *)"text"))) {
                // child = new struct node;
                grandchild = new XMLNode;
                grandchild->_name = reinterpret_cast<const char *>(lastchild->name);
                tmpstr = xmlNodeListGetString(_doc, lastchild->xmlChildrenNode, 1);
                // FIXME: get attributes for grandchildren too!
                if (tmpstr != 0) {
                  grandchild->_value = reinterpret_cast<const char *>(tmpstr);
                  log_msg("grandchild %s has data \"%s\"\n", grandchild->_name.c_str(), grandchild->_value.to_string());
                } else {
                  log_msg("Adding grandchild element %s\n", grandchild->_name.c_str());
                }
                attr = lastchild->properties;
                while (attr != NULL) {
                  log_msg("Node %s has property %s, value is %s\n",
                          grandchild->_name.c_str(), attr->name, attr->children->content);
                  XMLAttr *attrib = new XMLAttr;
                  attrib->_name = reinterpret_cast<const char *>(attr->name);
                  attrib->_value = reinterpret_cast<const char *>(attr->children->content);
                  grandchild->_attributes.push_back(attrib);
                  attr = attr->next;
                }
                child->_children.push_back(grandchild);
              }
              lastchild = lastchild->next; 
            }
          } else {
            tmpstr = xmlNodeListGetString(_doc, children->xmlChildrenNode, 1);
            child->_value = reinterpret_cast<const char *>(tmpstr);
            log_msg("Adding child element %s\n", child->_name.c_str());
          }
          element->_children.push_back(child);
#endif
        }
        children = children->next;
      }
      _nodes._children.push_back(element);
      
    }
    cur = cur->next;
  }
#endif
  
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

  return parseDoc(_doc);
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

  parseDoc(_doc);
  return true;
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

XMLNode *
XML::setupFrame(xmlnode_as_object *xml, as_environment *env)
{
  int child, i;
  tu_string nodename;
  as_value  nodevalue;
  as_value inum;
  xmlnode_as_object *xmlchildnode_obj;
  xmlnode_as_object *xmlnode_obj;
  xmlattr_as_object* attr_obj;
  
  xml_as_object *xobj = (xml_as_object *)xml;  
  
  XMLNode     *xmlnodes = xobj->obj.firstChild();
  while (xmlnodes != NULL) {
    // Get the data for this node
    nodename   = xmlnodes->_name;
    nodevalue  = xmlnodes->_value;
    
    // Create a new AS object for this node's children
    xmlnode_obj = new xmlnode_as_object;
    log_msg("Created XMLNode %s at 0x%X\n", nodename.c_str(), xmlnode_obj);
    xmlnode_obj->obj = xmlnodes;
    
    // Set these members in the top level object passed in.
    xobj->set_member("childNodes",  xmlnode_obj);         // the child nodes is the new object
    xobj->set_member("firstChild",  xobj);                // the first child is ourself
    xobj->set_member("length",      xmlnodes->length());  // the number of chldren we have
    xobj->set_member("nodeName",    nodename.c_str());    // all nodes have a name
    if (nodevalue.to_string()) {
      xmlnode_obj->set_member("nodeValue", nodevalue.to_string());
    }
    
    // Process the attributes, if any
    attr_obj = new xmlattr_as_object;
    for (i=0; i<xmlnodes->_attributes.size(); i++) {
    attr_obj->set_member(xmlnodes->_attributes[i]->_name, xmlnodes->_attributes[i]->_value);
    }
    xmlnode_obj->set_member("attributes", attr_obj);
    
    // Process the children, if there are any
    for (child=0; child<xmlnodes->length(); child++){
      nodename   = xmlnodes->_children[child]->_name;
      nodevalue  = xmlnodes->_children[child]->_value;
      xmlchildnode_obj = new xmlnode_as_object;
      log_msg("Created child XMLNode %s at 0x%X\n", nodename.c_str(), xmlchildnode_obj);
      xmlchildnode_obj->obj = xmlnodes->_children[child];
      //xmlnode_as_object *next = (xmlnode_as_object *)setupFrame((as_object *)xmlnode_obj);
      setupFrame(xmlchildnode_obj, env);
      inum = child;
      xmlnode_obj->set_member(inum.to_string(), xmlchildnode_obj);
    }
    
    return xmlnodes;
  }
}
  
// This sets up the stack frames for all the nodes from the XML file, so they
// can have the right vales in the right stack frame for the executing AS
// fucntion later.
void
XML::setupStackFrames(gameswf::as_object *xml, gameswf::as_environment *env)
{

  xml_as_object *xobj = (xml_as_object *)xml;
  //xmlnode_as_object *xmlnode_obj      = new xmlnode_as_object;
  xmlnode_as_object *xmlfirstnode_obj = new xmlnode_as_object;
  //xmlnode_as_object *xmlchildnode_obj = new xmlnode_as_object;
  xmlattr_as_object*	nattr_obj     = new xmlattr_as_object;

  int childa, childb, childc, childd, i, j, k;
  tu_string nodename;
  as_value  *nodevalue;
  int len;
  as_value inum;
  xmlnode_as_object *xmlnodeA_obj;
  xmlnode_as_object *xmlnodeB_obj;
  xmlnode_as_object *xmlnodeC_obj;
  xmlnode_as_object *xmlnodeD_obj;
  xmlnode_as_object *xmlnodeE_obj;
  xmlnode_as_object *xmlnodeF_obj;
  
  //log_msg("%s:\n",  __FUNCTION__);
  
  if (xml == 0) {
    log_error("No XML object!\n");
    return;
  }
  
  XMLNode *xmlnodes = xobj->obj.firstChild();

  xmlfirstnode_obj->obj = xmlnodes; // copy XML tree to XMLNode
  
  //log_msg("Created First XMLNode %s at 0x%X\n", xmlnodes->nodeName().c_str(), xmlnodes);
  
  // this is only used by network messages
  xml->set_member("firstChild", xmlfirstnode_obj);
  xml->set_member("childNodes",  xobj);
  xobj->set_member("length", xmlnodes->length());
  //xobj->set_member("length", xmlfirstnode_obj->obj.length());

  // This sets up the top level for parsing from memory
  for (i=0; i<xobj->obj.length(); i++) {
    xmlnode_as_object *xmlnode1_obj      = new xmlnode_as_object;
    //log_msg("Created XMLNode1 %s at 0x%X\n", xmlnodes->nodeName().c_str(), xmlnode1_obj );
    inum = i;
    xobj->set_member(inum.to_string(), xmlnode1_obj);
    xmlnode1_obj->set_member("nodeName", xmlnodes->_name.c_str());
    xmlnode1_obj->set_member("childNodes", xmlfirstnode_obj);
    xmlnode1_obj->set_member("length", xmlfirstnode_obj->obj.length());
    if (xmlnodes->_attributes.size() > 0) {
      xmlattr_as_object*	nattr_obj     = new xmlattr_as_object;
      for (j=0; j<xmlnodes->_attributes.size(); j++) {
        nattr_obj->set_member(xmlnodes->_attributes[j]->_name, xmlnodes->_attributes[j]->_value);
      }
      xmlnode1_obj->set_member("attributes", nattr_obj);
    }
  }
  
  // This is used when parsing from a text file
  nodename = xmlnodes->_name;
  xmlfirstnode_obj->set_member("nodeName", nodename.c_str());
  xmlfirstnode_obj->set_member("childNodes", xmlfirstnode_obj);
  xmlfirstnode_obj->set_member("length", xmlfirstnode_obj->obj.length());

  xmlattr_as_object*	attr_obj = new xmlattr_as_object;
  //log_msg("Created XMLAttr %s at 0x%X\n", xmlnodes->_attributes[i]->_name, attr_obj );

  for (i=0; i<xmlnodes->_attributes.size(); i++) {
    //log_msg("Created attribute %s at 0x%X for firstNode\n", xmlnodes->_attributes[i]->_name.c_str(), attr_obj);
    attr_obj->set_member(xmlnodes->_attributes[i]->_name, xmlnodes->_attributes[i]->_value);
  }
  xmlfirstnode_obj->set_member("attributes", attr_obj);
  
  for (childa=0; childa < xmlnodes->length(); childa++) {
    xmlnodeA_obj = new xmlnode_as_object;
    //log_msg("Created XMLNodeA %s at 0x%X\n", xmlnodes->nodeName().c_str(), xmlnodeA_obj );

    xmlnodeA_obj->obj = xmlnodes->_children[childa];

    inum = childa;
    nodename = xmlnodes->_children[childa]->nodeName().c_str();
    xmlfirstnode_obj->set_member(inum.to_string(), xmlnodeA_obj);
    
    xmlnodeA_obj->set_member("nodeName", nodename.c_str());
    //if (xmlnodes->_children[childa]->nodeValue()->size()) {
    //  xmlnodeA_obj->set_member("nodeValue", nodename.c_str());
    //}
    xmlnodeA_obj->set_member("length", xmlnodeA_obj->obj.length());
    if (xmlnodes->_children[childa]->_attributes.size() > 0) {
      xmlattr_as_object*	attr_obj     = new xmlattr_as_object;
      for (i=0; i<xmlnodes->_children[childa]->_attributes.size(); i++) {
        //log_msg("Created attribute %s at 0x%X for xmlnodeA\n",
        //        xmlnodes->_children[childa]->_attributes[i]->_name.c_str(), attr_obj);
        attr_obj->set_member(xmlnodes->_children[childa]->_attributes[i]->_name,
                             xmlnodes->_children[childa]->_attributes[i]->_value);
      }
      xmlnodeA_obj->set_member("attributes", attr_obj);
    }
    
    xmlnodeB_obj = new xmlnode_as_object;
    //log_msg("Created XMLNodeB %s at 0x%X\n",
    //        xmlnodes->_children[childa]->nodeName().c_str(), xmlnodeB_obj );
    xmlnodeA_obj->set_member("childNodes", xmlnodeB_obj);
    xmlnodeA_obj->set_member("firstChild", xmlnodeB_obj);
    //xmlnodeA_obj->set_member(inum.to_string(), xmlnodeB_obj);
    len =               xmlnodes->_children[childa]->length();
    xmlnodeB_obj->set_member("length", len);
    xmlnodeB_obj->set_member("firstChild", xmlnodeB_obj);
    if (xmlnodes->_children[childa]->_attributes.size() > 0) {
      xmlattr_as_object*	attr_obj     = new xmlattr_as_object;
      for (i=0; i<xmlnodes->_children[childa]->_attributes.size(); i++) {
        //log_msg("Created attribute %s at 0x%X for xmlnodeB\n",
        //        xmlnodes->_children[childa]->_attributes[i]->_name.c_str(), attr_obj);
        attr_obj->set_member(xmlnodes->_children[childa]->_attributes[i]->_name,
                             xmlnodes->_children[childa]->_attributes[i]->_value);
      }
      xmlnodeB_obj->set_member("attributes", attr_obj);
    }
    for (childb=0; childb < xmlnodeA_obj->obj.length(); childb++) {
      xmlnodeC_obj = new xmlnode_as_object;
      //og_msg("Created XMLNodeC %s at 0x%X\n", xmlnodes->_children[childa]->_children[childb]->nodeName().c_str(), xmlnodeC_obj);
      xmlnodeC_obj->obj = xmlnodes->_children[childa]->_children[childb];
      nodename =          xmlnodes->_children[childa]->_children[childb]->nodeName().c_str();
      nodevalue =         xmlnodes->_children[childa]->_children[childb]->nodeValue();
      
      inum = childb;
      xmlnodeB_obj->set_member(inum.to_string(), xmlnodeC_obj);
      xmlnodeC_obj->set_member("childNodes", xmlnodeC_obj);
      xmlnodeC_obj->set_member("firstChild", xmlnodeC_obj);
      xmlnodeC_obj->set_member("nodeName", nodename.c_str());
      if (nodevalue->to_string()) {
        xmlnodeC_obj->set_member("nodeValue", nodevalue->to_string());
      }
      if (xmlnodes->_children[childa]->_children[childb]->_attributes.size() > 0) {
        for (i=0; i<xmlnodes->_children[childa]->_attributes.size(); i++) {
        xmlattr_as_object*	attr_obj     = new xmlattr_as_object;
        //log_msg("Created attribute %s at 0x%X for xmlnodeC\n", xmlnodes->_children[childa]->_children[childb]->_attributes[i]->_name.c_str(), attr_obj);
        attr_obj->set_member(xmlnodes->_children[childa]->_children[childb]->_attributes[i]->_name,
                             xmlnodes->_children[childa]->_children[childb]->_attributes[i]->_value);
        }
        xmlnodeC_obj->set_member("attributes", attr_obj);
      }

      xmlnodeC_obj->set_member("length", xmlnodeC_obj->obj.length());
      for (childc=0; childc <  xmlnodeC_obj->obj.length(); childc++) {
        
        xmlnodeD_obj = new xmlnode_as_object;
        xmlnodeD_obj->obj = xmlnodes->_children[childa]->_children[childb]->_children[childc];
        nodename =          xmlnodes->_children[childa]->_children[childb]->_children[childc]->nodeName().c_str();
        nodevalue =         xmlnodes->_children[childa]->_children[childb]->_children[childc]->nodeValue();
        //log_msg("Created XMLNodeD %s at 0x%X\n", nodename.c_str(), xmlnodeD_obj);
        inum = childc;
        xmlnodeC_obj->set_member(inum.to_string(), xmlnodeD_obj);
        
        xmlnodeD_obj->set_member("childNodes", xmlnodeD_obj);
        xmlnodeD_obj->set_member("firstChild", xmlnodeD_obj);
        xmlnodeD_obj->set_member("nodeName", nodename.c_str());
        xmlnodeD_obj->set_member("length", xmlnodeD_obj->obj.length());

        if (nodevalue->to_string()) {
          xmlnodeD_obj->set_member("nodeValue", nodevalue->to_string());
        }
        if (xmlnodes->_children[childa]->_children[childb]->length() > 0) {
          if (xmlnodes->_children[childa]->_children[childb]->_children[childc]->_attributes.size() > 0) {
            xmlattr_as_object*	attr_obj     = new xmlattr_as_object;
            for (k=0; k<xmlnodes->_children[childa]->_children[childb]->_children[childc]->_attributes.size(); k++) {
              //log_msg("Created attribute %s at 0x%X for xmlnodeD\n",
              //        xmlnodes->_children[childa]->_children[childb]->_children[childc]->_attributes[i]->_name.c_str(), attr_obj);
              attr_obj->set_member(xmlnodes->_children[childa]->_children[childb]->_children[childc]->_attributes[k]->_name,
                                   xmlnodes->_children[childa]->_children[childb]->_children[childc]->_attributes[k]->_value);
            }
            xmlnodeD_obj->set_member("attributes", attr_obj);
          }
        }
        
        for (childd=0; childd <  xmlnodeD_obj->obj.length(); childd++) {
          xmlnodeE_obj = new xmlnode_as_object;
          nodename =  xmlnodes->_children[childa]->_children[childb]->_children[childc]->_children[childd]->nodeName().c_str();
          nodevalue = xmlnodes->_children[childa]->_children[childb]->_children[childc]->_children[childd]->nodeValue();
          xmlnodeE_obj->obj = xmlnodes->_children[childa]->_children[childb]->_children[childc]->_children[childd];
          //log_msg("Created XMLNodeE %s at 0x%X, value %s\n",
          //        nodename.c_str(), xmlnodeE_obj, nodevalue->to_string());
          xmlnodeE_obj->set_member("firstChild", xmlnodeE_obj);
          xmlnodeE_obj->set_member("nodeName", nodename.c_str());
          if (nodevalue->to_string()) {
            xmlnodeE_obj->set_member("nodeValue", nodevalue->to_string());
          }
          inum = childd;
          xmlnodeD_obj->set_member(inum.to_string(), xmlnodeE_obj);
          
          if (xmlnodes->_children[childa]->_children[childb]->_children[childc]->_children[childd]->_attributes.size() > 0) {
            xmlattr_as_object*	attr_obj     = new xmlattr_as_object;
            for (i=0; i<xmlnodes->_children[childa]->_children[childb]->_children[childc]->_children[childd]->_attributes.size(); i++) {
              //log_msg("Created attribute %s at 0x%X for xmlnodeE\n",
              //        xmlnodes->_children[childa]->_children[childb]->_children[childc]->_children[childd]->_attributes[i]->_name.c_str(), attr_obj);
              attr_obj->set_member(xmlnodes->_children[childa]->_children[childb]->_children[childc]->_children[childd]->_attributes[i]->_name,
                                   xmlnodes->_children[childa]->_children[childb]->_children[childc]->_children[childd]->_attributes[i]->_value);
            }
            xmlnodeE_obj->set_member("attributes", attr_obj);
    }

        }
        //xmlnodeB_obj->set_member("length", xmlnodeB_obj->obj.length());      
      }
      xmlnodeB_obj->set_member("childNodes", xmlnodeC_obj);
    }
  }
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


  log_msg("%s:\n", __FUNCTION__);
  
  xml_as_object*	ptr = (xml_as_object*) (as_object*) this_ptr;
  xmlnode_as_object*	node = new xmlnode_as_object;
  
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
  ret = ptr->obj.load(filespec);
  result->set(ret);

  if (ret == false) {
    return;
  }
    
  //env->bottom(first_arg) = ret;
  array<with_stack_entry> with_stack;
  array<with_stack_entry> dummy_stack;
  //  struct node *first_node = ptr->obj.firstChildGet();
  
  //const char *name = ptr->obj.nodeNameGet();

  if (ptr->obj.hasChildNodes() == false) {
    log_error("%s: No child nodes!\n", __FUNCTION__);
  }
  
  array<XMLNode *> childnodes = ptr->obj.childNodes();
  //  node
  node->obj = *ptr->obj.firstChild();

// FIXME: decide...
#if 1
  // The old way
  ptr->obj.setupStackFrames(ptr, env);
#else
  // The new recursive way
  ptr->obj.setupFrame(node, env);
#endif
  
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
        (*func)(&val, node, env, nargs, first_arg); // was this_ptr instead of node
		}
    else if (as_as_function* as_func = method.to_as_function())
      {
        // It's an ActionScript function.  Call it.
        log_msg("Calling ActionScript function for onLoad\n");
        (*as_func)(&val, node, env, nargs, first_arg); // was this_ptr instead of node
      }
    else
      {
        log_error("error in call_method(): method is not a function\n");
      }    
  } else {
    log_msg("Couldn't find onLoad event handler, setting up callback\n");
    ptr->set_event_handler(event_id::XML_LOAD,
                               (as_c_function_ptr)&xml_onload);
  }
#else
  ptr->set_event_handler(event_id::XML_LOAD,
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
  // log_msg("%s:\n", __FUNCTION__);
    
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
  int           i;
  as_value      inum;
  xml_as_object *xml_obj;
  
  log_msg("%s: nargs=%d\n", __FUNCTION__, nargs);
  
  if (nargs > 0) {
    if (env->top(0).get_type() == as_value::STRING) {
      xml_obj = new xml_as_object;
      //log_msg("\tCreated New XML object at 0x%X\n", (unsigned int)xml_obj);
      tu_string datain = env->top(0).to_tu_string();
      xml_obj->obj.parseXML(datain);
      xml_obj->obj.setupStackFrames(xml_obj, env);
    } else {
      xmlnode_as_object*	xmlnode_obj = (xmlnode_as_object*)env->top(0).to_object();      
      //log_msg("\tCloned the XMLNode object at 0x%X\n", (unsigned int)xml_obj);
      result->set(xmlnode_obj);
      return;
    }
  } else {
    xml_obj = new xml_as_object;
    xml_obj->set_member("load", &xml_load);
    xml_obj->set_member("loaded", &xml_loaded);
  }
  
  result->set(xml_obj);  
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
