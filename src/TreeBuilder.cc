#include "TreeBuilder.h"

TreeBuilder::TreeBuilder() = default;
TreeBuilder::~TreeBuilder() = default;

std::string TreeBuilder::GenerateJson() {
  std::string result;
  result += "{";
  for (size_t i = 0; i < elements_.size(); i++) {
    result += elements_[i].GenerateJson();
    if (i+1 < elements_.size()) { // not the last one
      result += ",";
    }
  }
  result += "}";

  return result;
}

void TreeBuilder::EndOfChildren() {
  if (nested_elements_.empty()) {
    // fprintf(stderr, "Found the end of the childrens but we don't have any element\n");
    // TODO: HOW?!
    return;
  }
  
  // The next children will be about the previous element
  nested_elements_.pop_back();

  // Update the type of the current element
  if (!nested_elements_.empty()) {
    last_parsed_type_ = elements_[nested_elements_.back()].type_;
  }
}

void TreeBuilder::AddElement(ElementType element_type, uint64_t tag_id, bool has_children) {
  bool element_added = false;

  ElementType current_element_type = ElementType::none;
  if (!nested_elements_.empty()) {
    current_element_type = elements_[nested_elements_.back()].type_;
  }

  switch(element_type) {
    case ElementType::none:
      break;
    case ElementType::member:         // Member
      if (current_element_type == ElementType::none) {
        break;
      }
      if (nested_elements_.empty()) {
        fprintf(stderr, "Can't add a member if the element list is empty\n");
        break;
      }
      elements_[nested_elements_.back()].members_.push_back(Element(element_type, tag_id));
      last_parsed_type_ = element_type;
      element_added = true;
      break;
    case ElementType::inheritance:    // Parent
      if (current_element_type == ElementType::none) {
        break;
      }  
      if (nested_elements_.empty()) {
        fprintf(stderr, "Can't add a parent if the element list is empty\n");
        break;
      }
      elements_[nested_elements_.back()].parents_.push_back({tag_id, 0});
      last_parsed_type_ = element_type;
      element_added = true;
      break;
    case ElementType::subrange_type:  // Subrange
      break;                          // Just update the current element type
    default:
      elements_.push_back(Element(element_type, tag_id));
      last_parsed_type_ = element_type;
      element_added = true;
  }

  if (has_children) {
    if (!element_added) {
      // Add a useless element to keep the stack in a correct state
      elements_.push_back(Element(ElementType::none, 0));
    }
    nested_elements_.push_back(elements_.size()-1);
  }
}

void TreeBuilder::SetElementName(const char* name) {
  if (elements_.empty()) {
    return;
  }

  if (last_parsed_type_ == ElementType::none) {
    return;
  }

  if (last_parsed_type_ == ElementType::member) {
    if (nested_elements_.empty()) {
      return;
    }
    Element& current_element = elements_[nested_elements_.back()];
    if (current_element.type_ == ElementType::none) {
      return; // don't update this element
    }

    if (current_element.members_.empty()) {
      fprintf(stderr, "Can't set the member name if the members list is empty\n");
      return;
    }

    current_element.members_.back().name_ = name;
    return;
  }

  elements_.back().name_ = name;
}

void TreeBuilder::SetElementSize(uint64_t size) {
  if (elements_.empty()) {
    return;
  }
  if (last_parsed_type_ == ElementType::none) {
    return;
  }

  if (last_parsed_type_ == ElementType::member) {
    if (nested_elements_.empty()) {
      return;
    }
    Element& current_element = elements_[nested_elements_.back()];
    if (current_element.type_ == ElementType::none) {
      return; // don't update this element
    }

    if (current_element.members_.empty()) {
      fprintf(stderr, "Can't set the member size if the members list is "
        "empty\n");
      return;
    }

    current_element.members_.back().size_ = size;
    return;
  }

  elements_.back().size_ = size; 
}

void TreeBuilder::SetElementOffset(uint64_t offset) {
  if (elements_.empty()) {
    return;
  }
  if (last_parsed_type_ == ElementType::none) {
    return;
  }

  switch (last_parsed_type_) {
    case ElementType::member: {
      if (nested_elements_.empty()) {
        break;
      }
      Element& current_element = elements_[nested_elements_.back()];
      if (current_element.type_ == ElementType::none) {
        break; // don't update this element
      }

      if (current_element.members_.empty()) {
        fprintf(stderr, "Can't set the member offset if the members list is "
          "empty\n");
        break;
      }
      current_element.members_.back().offset_ = offset;
      break;
    }
    case ElementType::inheritance: {
      if (nested_elements_.empty()) {
        break;
      }
      Element& current_element = elements_[nested_elements_.back()];
      if (current_element.type_ == ElementType::none) {
        break; // don't update this element
      }

      if (current_element.parents_.empty()) {
        fprintf(stderr, "Can't set the parent offset if the parents list is "
          "empty\n");
        break;
      }
      current_element.parents_.back().offset = offset;
      break;
    }
    default:
      break;
  }
}

void TreeBuilder::SetElementType(uint64_t type_id) {
  if (elements_.empty()) {
    return;
  }
  if (last_parsed_type_ == ElementType::none) {
    return;
  }

  switch (last_parsed_type_) {
    case ElementType::member: {
      if (nested_elements_.empty()) {
        break;
      }
      Element& current_element = elements_[nested_elements_.back()];
      if (current_element.type_ == ElementType::none) {
        break; // don't update this element
      }

      if (current_element.members_.empty()) {
        fprintf(stderr, "Can't set the member type if the members list is "
          "empty\n");
        break;
      }
      current_element.members_.back().type_id_ = type_id;
      break;
    }
    case ElementType::inheritance: {
      if (nested_elements_.empty()) {
        break;
      }
      Element& current_element = elements_[nested_elements_.back()];
      if (current_element.type_ == ElementType::none) {
        break; // don't update this element
      }

      if (current_element.parents_.empty()) {
        fprintf(stderr, "Can't set the parent type if the parents list is "
          "empty\n");
        break;
      }
      elements_[nested_elements_.back()].parents_.back().id = type_id;
      break;
    }
    case ElementType::subrange_type:
      break; // do nothing
    default:
      elements_.back().type_id_ = type_id;
      break;
  }
}

void TreeBuilder::SetElementCount(uint64_t count) {
  if (elements_.empty()) {
    return;
  }
  if (last_parsed_type_ != ElementType::subrange_type) {
    return;
  }
  elements_.back().count_ = count;
}

// static
std::string TreeBuilder::EscapeJsonString(const char* str) {
  std::string result;
  size_t str_len = std::char_traits<char>::length(str); 

  for (size_t i = 0; i < str_len; i++) {
    if (str[i] == '\\' || str[i] == '"') {
      result += '\\';
    }

    result += str[i];
  }

  return result;
}

const char* TreeBuilder::Element::TypeName() {
  switch (type_) {
    case ElementType::none: return "none";
    case ElementType::array_type: return "array";
    case ElementType::class_type: return "class";
    case ElementType::enumerator_type: return "enumerator";
    case ElementType::member: return "member";
    case ElementType::pointer_type: return "pointer";
    case ElementType::structure_type: return "structure";
    case ElementType::typedef2: return "typedef";
    case ElementType::union_type: return "union";
    case ElementType::inheritance: return "inheritance";
    case ElementType::base_type: return "base";
    case ElementType::const_type: return "const";
    default: return "unk";
  }
}

std::string TreeBuilder::Element::GenerateJson() {
  std::string result;

  // A member is a special case
  if (type_ == ElementType::member) {
    result = "{";

    if (type_id_) {
      result += "\"type_id\":\""+std::to_string(type_id_)+"\",";
    }
    if (name_) {
      result += "\"name\":\""+EscapeJsonString(name_)+"\",";
    }
    result += "\"offset\":"+std::to_string(offset_);

    result += "}";
    return result;
  }

  // The others are generic
  result += "\""+std::to_string(id_)+"\":";
  result += "{\"type\":\""+std::string(TypeName())+"\",";
  if (type_id_) {
    result += "\"type_id\":\""+std::to_string(type_id_)+"\",";
  }
  if (name_) {
    result += "\"name\":\""+EscapeJsonString(name_)+"\",";
  }
  if (size_) {
    result += "\"size\":"+std::to_string(size_)+",";
  }
  if (count_) {
    result += "\"count\":"+std::to_string(count_)+",";
  }
  if (parents_.size() > 0) {
    result += "\"parents\":[";
    for (size_t i = 0; i < parents_.size(); i++) {
      result += "{\"id\":\""+std::to_string(parents_[i].id)+"\",";
      result += "\"offset\":"+std::to_string(parents_[i].offset)+"}";
      if (i+1 < parents_.size()) {
        result += ",";
      }
    }
    result += "],";
  }
  if (members_.size() > 0) {
    result += "\"members\":[";
    for (size_t i = 0; i < members_.size(); i++) {
      result += members_[i].GenerateJson();
      if (i+1 < members_.size()) {
        result += ",";
      }
    }
    result += "],";
  }

  if (result.back() == ',') {
    result.pop_back();
  }
  result += "}";
  return result;
}