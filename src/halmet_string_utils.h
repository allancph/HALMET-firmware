#ifndef HALMET_STRING_UTILS_H_
#define HALMET_STRING_UTILS_H_

#include <WString.h>

namespace halmet {

String getConfigPath(const String& type_plural_capitalized, const String& name,
                     const String& attribute);
String getTitle(const String& type_singular_capitalized, const String& name,
                const String& attribute);
String getDescription(const String& text_before_name, const String& name,
                      const String& text_after_name_if_any);
String getSkPath(const String& sk_type, const String& sk_id,
                 const String& attribute);
String getMetaDisplayName(const String& text_before_name, const String& name,
                          const String& text_after_name_if_any);
String getMetaDescription(const String& text_before_name, const String& name,
                          const String& text_after_name_if_any);

}  // namespace halmet

#endif  // HALMET_STRING_UTILS_H_
