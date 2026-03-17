import { useState } from 'react';
import toast from 'react-hot-toast';
import { postFeed } from '../../services/api';

export default function FeedButton() {
    const [loading, setLoading] = useState(false);

    const handleFeed = async () => {
        if (loading) return;
        setLoading(true);
        const toastId = toast.loading('Sending feed command…');
        try {
            await postFeed();
            toast.success('Feed command sent! 🐱', { id: toastId });
        } catch (err) {
            toast.error(`Failed: ${err.message}`, { id: toastId });
        } finally {
            setLoading(false);
        }
    };

    return (
        <button
            id="btn-manual-feed"
            onClick={handleFeed}
            disabled={loading}
            className="btn-primary flex items-center gap-2.5 px-6 py-3 text-base w-full justify-center"
        >
            {loading ? (
                <>
                    <svg className="animate-spin w-5 h-5" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                        <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
                        <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8z" />
                    </svg>
                    Sending…
                </>
            ) : (
                <>
                    <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                        <polygon points="5 3 19 12 5 21 5 3" />
                    </svg>
                    Manual Feed Now
                </>
            )}
        </button>
    );
}
