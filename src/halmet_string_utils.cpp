#include "halmet_string_utils.h"

namespace halmet {

String getConfigPath(const String& type_plural_capitalized, const String& name,
                     const String& attribute) {
  return "/" + type_plural_capitalized + "/" + name + "/" + attribute;
}

String getTitle(const String& type_singular_capitalized, const String& name,
                const String& attribute) {
  return name + " " + type_singular_capitalized + " " + attribute;
}

String getDescription(const String& text_before_name, const String& name,
                      const String& text_after_name_if_any) {
  String desc = text_before_name;
  if (!text_before_name.isEmpty() && !text_before_name.endsWith(" ")) {
    desc += " ";
  }
  desc += name;
  if (!text_after_name_if_any.isEmpty()) {
    if (!text_after_name_if_any.startsWith(" ")) {
      desc += " ";
    }
    desc += text_after_name_if_any;
  }
  return desc;
}

String getSkPath(const String& sk_type, const String& sk_id,
                 const String& attribute) {
  if (sk_type == "alarm" && attribute.isEmpty()) {
    return sk_type + "." + sk_id;
  }
  return sk_type + "." + sk_id + "." + attribute;
}

String getMetaDisplayName(const String& text_before_name, const String& name,
                          const String& text_after_name_if_any) {
  String disp_name = text_before_name;
  if (!text_before_name.isEmpty() && !text_before_name.endsWith(" ")) {
    disp_name += " ";
  }
  disp_name += name;
  if (!text_after_name_if_any.isEmpty()) {
    if (!text_after_name_if_any.startsWith(" ")) {
      disp_name += " ";
    }
    disp_name += text_after_name_if_any;
  }
  return disp_name;
}

String getMetaDescription(const String& text_before_name, const String& name,
                          const String& text_after_name_if_any) {
  String meta_desc = text_before_name;
  if (!text_before_name.isEmpty() && !text_before_name.endsWith(" ")) {
    meta_desc += " ";
  }
  meta_desc += name;
  if (!text_after_name_if_any.isEmpty()) {
    if (!text_after_name_if_any.startsWith(" ")) {
      meta_desc += " ";
    }
    meta_desc += text_after_name_if_any;
  }
  return meta_desc;
}

}  // namespace halmet
