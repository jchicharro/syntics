while (pos < string.size())
{
    std::string tp;
    tp += string[pos];
    //print(tp + " " + std::to_string(pos));
    bool parsing_stopped = stopper_pair != -1;
    
    if (parsing_stopped)
    {
        if (int advance = resume_parsing(pos))
        {
            dataAppend(it.node->data,str(string.substr(pos,advance)));
            pos += advance;
        }
        else
        {
            dataAppend(it.node->data,str(string[pos]));
            pos += 1;
        }
        continue;
    }
    it++;
    ++it;
}
