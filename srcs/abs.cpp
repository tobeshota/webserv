#include "webserv.hpp"

namespace ft {
// ソースコードが存在しない際に，単体テストの分岐網羅率を100%にするためのコード．
// いずれ削除する
int abs(int a) {
  if (a < 0) {
    a *= -1;
  }
  return a;
}

}  // namespace ft
