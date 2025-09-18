// //
// // Created by niooi on 9/7/2025.
// //
//
// #pragma once
//
// #include <engine/contexts/net/channel.h>
// #include <string>
//
// namespace v {
//     /// simple chat channel for testing network stuff
//     class ChatChannel : public NetChannel<ChatChannel, std::string> {
//     public:
//         using PayloadT = std::string;
//
//         ChatChannel() = default;
//
//         static std::string parse(const u8* bytes, u64 len)
//         {
//             const u8* string_data = bytes;
//             u64       string_len  = len;
//
//             return std::string(reinterpret_cast<const char*>(string_data), string_len);
//         }
//     };
// } // namespace v
