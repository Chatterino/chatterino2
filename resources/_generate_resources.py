resources_header = \
'''<RCC>
  <qresource prefix="/">'''

resources_footer = \
'''  </qresource>
</RCC>'''

header_header = \
'''#include <QPixmap>
#include "common/Singleton.hpp"

namespace chatterino {

class Resources2 : public Singleton {
public:
    Resources2();

'''

header_footer = \
'''};

}  // namespace chatterino'''

source_header = \
'''#include "ResourcesAutogen.hpp"

namespace chatterino {

Resources2::Resources2()
{
'''

source_footer = \
'''}

}  // namespace chatterino'''
