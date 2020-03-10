#include <nds/graph/node.hpp>
#include <memory>
struct node;
namespace nds::internal
{
    //! T type to store as node<T>, T must exists in the graph
    //! Base base type of T to store as node<Base> if T is not in the graph
    template<class... Ts, class... Us, class... Vs>
    template<class B, class T>
    auto graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::add(T v)
    {
        // Base = T if Base is not specified
        using Base = std::conditional_t<std::is_same_v<B, void>, T, B>;

        constexpr int type_index = cx::index_of<std::vector<internal_node_ptr<Base>>, node_container_type>::value;

        auto target_node = basic_node<T, Base>{ std::move(v) };
        std::unique_ptr<basic_node<T, Base>> ptr = std::make_unique<basic_node<T, Base>>(std::move(target_node));
        auto last_node = ptr.get();

        std::get<type_index>(nodes_).emplace_back(std::move(ptr));

        return node_ptr<Base>(last_node);
    }

    template<class... Ts, class... Us, class... Vs>
    template<class B, class T, class Source>
    auto graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::add(T v, node_ptr<Source> source)
    {
        auto last_node = add<B, T>(std::move(v));
        connect(source, last_node);
        return last_node;
    }

    template<class... Ts, class... Us, class... Vs>
    template<class B, class T, class... Args, disable_node<Args...>>
    auto graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::emplace(Args&&... args)
    {
        constexpr int type_index = cx::index_of<std::vector<internal_node_ptr<B>>, node_container_type>::value;

        auto ptr = std::make_unique<basic_node<T, B>>( std::forward<Args>(args)... );
        auto last_node = ptr.get();

        std::get<type_index>(nodes_).emplace_back(std::move(ptr));

        return node_ptr<B>(last_node);
    }

    template<class... Ts, class... Us, class... Vs>
    template<class B, class T, class Source, class... Args>
    auto graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::emplace(node_ptr<Source> source, Args&&... args)
    {
        auto last_node = emplace<B, T>(std::forward<Args>(args)...);
        connect(source, last_node);
        return last_node;
    }

    template<class... Ts, class... Us, class... Vs>
    template<class Source_type, class Target_type>
    void graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::connect(node_ptr<Source_type> source_, node_ptr<Target_type> target_)
    {
        using Source = std::remove_const_t<Source_type>;
        using Target = std::remove_const_t<Target_type>;
        node_ptr<Source> source{ const_cast<node_type<Source>*>(source_.get()) };
        node_ptr<Target> target{ const_cast<node_type<Target>*>(target_.get()) };

        if (!source || !target) return;
        // check edges
        constexpr int type_index = cx::index_of<std::vector<edge<node_ptr<Source>, node_ptr<Target>>>, edge_container_type>::value;
        // static_assert(index >= 0, "connection between U and V not allowed");

        std::get<type_index>(edges_).emplace_back(edge<node_ptr<Source>, node_ptr<Target>>{ source, target });
    }

    template<class... Ts, class... Us, class... Vs>
    template<class F>
    void graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::nodes(F&& f) const
    {
        nodes<nodes_type>(std::forward<F>(f));
    }

    template<class... Ts, class... Us, class... Vs>
    template<class Nodes, class F>
    void graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::nodes(F&& f) const
    {
        nds::cx::for_each<Nodes>([&f, this](auto&& nt)
        {
            using input_node_ptr = node_ptr<typename std::decay_t<decltype(nt)>::type>;

            auto loop_graph_type = [&f](auto&& vector)
            {
                using graph_node_ptr = typename std::decay_t<decltype(vector)>::value_type::element_type; // node_ptr<T>
                if constexpr (std::is_same_v<input_node_ptr, graph_node_ptr>)
                {
                    for (auto&& node : vector) f(node);
                }
            };

            std::apply([&](auto&&... vectors) { (loop_graph_type(vectors), ...); }, nodes_);
        });
    }

    template<class... Ts, class... Us, class... Vs>
    template<class F>
    void graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::edges(F&& f) const
    {
        edges<edges_type>(std::forward<F>(f));
    }

    template<class... Ts, class... Us, class... Vs>
    template<class Edges, class F>
    void graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::edges(F&& f) const
    {
        nds::cx::for_each<Edges>([&f, this](auto&& et)
        {
            using input_raw_edge_type = typename std::decay_t<decltype(et)>::type; // nds::edge<U, V>
            using input_edge_type = nds::edge<node_ptr<typename input_raw_edge_type::source_type>, node_ptr<typename input_raw_edge_type::target_type>>;

            auto loop_edge_type = [&f](auto&& vector)
            {
                using graph_edge_type = typename std::decay_t<decltype(vector)>::value_type; // edge<node_ptr<U>, node_ptr<V>>
                if constexpr (std::is_same_v<input_edge_type, graph_edge_type>)
                {
                    for (auto&& edge : vector) f(edge);
                }
            };

            std::apply([&](auto&&... vectors) { (loop_edge_type(vectors), ...); }, edges_);
        });
    }

    template<class... Ts, class... Us, class... Vs>
    template<class Target, class F>
    void graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::sources(node_ptr<Target> target, F&& f)
    {
        if (!target) return;
        auto loop_graph_edge = [&](auto&& vector)
        {
            using graph_edge_type = typename std::decay_t<decltype(vector)>::value_type::source_type; // node_ptr<T>

            for (auto&& edge : vector)
            {
                if (target.id() == edge.target.id()) f(edge.source);
            }
        };

        std::apply([&](auto&&... vectors) { (loop_graph_edge(vectors), ...); }, edges_);
    }

    template<class... Ts, class... Us, class... Vs>
    template<class Source, class F>
    void graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::targets(node_ptr<Source> source, F&& f)
    {
        if (!source) return;
        auto loop_graph_edge = [&](auto&& vector)
        {
            using graph_edge_type = typename std::decay_t<decltype(vector)>::value_type::source_type; // node_ptr<T>

            for (auto&& edge : vector)
            {
                if (source.id() == edge.source.id()) f(edge.target);
            }
        };

        std::apply([&](auto&&... vectors) { (loop_graph_edge(vectors), ...); }, edges_);
    }

    template<class... Ts, class... Us, class... Vs>
    std::size_t graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::count_nodes() const
    {
        return std::apply([&](auto&&... vectors) { return (vectors.size() + ...); }, nodes_);
    }
    template<class... Ts, class... Us, class... Vs>
    std::size_t graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::count_edges() const
    {
        return std::apply([&](auto&&... vectors) { return (vectors.size() + ...); }, edges_);
    }

    template<class... Ts, class... Us, class... Vs>
    constexpr std::size_t graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::count_nodes_type()
    {
        return sizeof...(Ts);
    }
    template<class... Ts, class... Us, class... Vs>
    constexpr std::size_t graph<graph_types<Ts...>, graph_edges<edge<Us, Vs>...>, graph_storages::tuple_vector>
    ::count_edges_type()
    {
        return 0;//sizeof...(edge<Us..., Vs...>);
    }

    /*



    template<class... Ts, class... Us, class... Vs>
    template<class Source, class Target>
    void graph<graph_types<Ts...>, nds::graph_edges<edge<Us, Vs>...>>
    ::connect(Source* source, Target* target)
    {
        // check edges
        constexpr int index = cx::index_of<std::vector<edge<Source, Target>>, edge_container_type>::value;
        // static_assert(index >= 0, "connection between U and V not allowed");

        source.targets_count++;

        std::get<index>(edges_).emplace_back(edge<Source, Target>{ source, target });
    }

    template<class... Ts, class... Us, class... Vs>
    template<class T>
    void graph<graph_types<Ts...>, nds::graph_edges<edge<Us, Vs>...>>
    ::connect(int s, int t)
    {
        // check edges
        constexpr int index = cx::index_of<std::vector<edge<node_ptr<T>, node_ptr<T>>>, edge_container_type>::value;
        // static_assert(index >= 0, "connection between U and V not allowed");
        node_ptr<T>* source = std::get<0>(nodes_)[s].get();
        node_ptr<T>* target = std::get<0>(nodes_)[t].get();
        std::get<index>(edges_).emplace_back(edge<node_ptr<T>, node_ptr<T>>{ source, target });
    }
     */
} // nds::internal