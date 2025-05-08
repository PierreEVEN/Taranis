#include "llp/parser.hpp"

#include "llp/native_tokens.hpp"

namespace Llp
{
Parser::Parser(const TokenizedBlock& in_block) : block(&in_block)
{
    tokens_to_skip = {TTokenType<WhitespaceToken>::id, TTokenType<CommentToken>::id, TTokenType<EndlToken>::id};
}
} // namespace Llp