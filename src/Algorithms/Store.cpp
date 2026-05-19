#include <Algorithms/Store.h>

void
Store::encode(std::istream& file, std::ostream& out)
{
    out << file.rdbuf();
}

void
Store::decode(BoundedStream& in, std::ostream& file)
{
    char buffer[4096];
    while (!in.reached_limit()) {
        const std::streamsize read = in.read_bounded(buffer, sizeof(buffer));
        if (read <= 0)
            break;
        file.write(buffer, read);
    }
}
